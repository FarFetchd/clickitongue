#include "fft_result_distributor.h"

#include <cstdio>

void safelyExit(int exit_code);

#ifdef CLICKITONGUE_LINUX
#include <unistd.h>
#include <sys/wait.h>
extern char* g_program_path;
void restartProgram()
{
  // actually it looks like the below causes multiple duplicate copies to run, +1 per sleep, soooo....
  safelyExit(0); // just exit for now :(

  // pid_t pid = fork();
  // if (pid < 0) // failed
  //   safelyExit(1);
  // if (pid > 0) // we are the parent
  // {
  //   waitpid(pid, nullptr, 0);
  //   safelyExit(0);
  // }
  // // we must be the child
  // execl(g_program_path, g_program_path, (char*)nullptr);
  // // If execl returns, it means an error occurred
  // safelyExit(1);
}

extern volatile sig_atomic_t g_shutdown_flag;
#include <thread>
void watchdog(FFTResultDistributor* distrib)
{
  std::this_thread::sleep_for(std::chrono::seconds(1));
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    uint64_t dog_time = distrib->watchdog_time_;
    uint64_t cur_time = std::chrono::duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
    if (g_shutdown_flag)
      safelyExit(0);
    if (cur_time - dog_time > 1)
      restartProgram();
  }
}
#endif

FFTResultDistributor::FFTResultDistributor(
    std::vector<std::unique_ptr<Detector>>&& detectors,
    double scale, bool training)
: detectors_(std::move(detectors)), fft_lease_(g_fourier->borrowWorker()),
  scale_(scale), training_(training)
{
#ifdef CLICKITONGUE_LINUX
  std::thread(watchdog, this).detach();
#endif
}

void FFTResultDistributor::
replaceDetectors(std::vector<std::unique_ptr<Detector>>&& detectors)
{
  detectors_ = std::move(detectors);
}

bool g_show_debug_info = false;
void FFTResultDistributor::processAudio(const Sample* cur_sample, int num_frames)
{
  if (num_frames != kFourierBlocksize)
  {
    printf("illegal num_frames: expected %d, got %d\n", kFourierBlocksize, num_frames);
    safelyExit(1);
  }

  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (g_num_channels == 2)
      fft_lease_.in[i] = (cur_sample[i*g_num_channels] + cur_sample[i*g_num_channels+1]) / 2.0;
    else
      fft_lease_.in[i] = cur_sample[i];
  }
  fft_lease_.runFFT();

  for (int i=0; i<kNumFourierBins; i++)
  {
    fft_lease_.out[i][0] = scale_ * (fft_lease_.out[i][0]*fft_lease_.out[i][0] +
                                     fft_lease_.out[i][1]*fft_lease_.out[i][1]);
  }
  for (auto& detector : detectors_)
    detector->processFourierOutputBlock(fft_lease_.out);
  if (g_show_debug_info && !training_)
    g_fourier->printOctavesAlreadyFreq(fft_lease_.out);
}

int fftDistributorCallback(const void* input, void* output,
                           unsigned long num_frames,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags, void* user_data)
{
  FFTResultDistributor* distrib = static_cast<FFTResultDistributor*>(user_data);
  const Sample* cur_samples = static_cast<const Sample*>(input);
  if (cur_samples)
    distrib->processAudio(cur_samples, num_frames);
#ifdef CLICKITONGUE_LINUX
  distrib->watchdog_time_ = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
#endif
  return paContinue;
}
