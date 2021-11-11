#include "easy_fourier.h"

#include <cmath>
#include <thread>

#include "config_io.h"
#include "constants.h"

EasyFourier* g_fourier = nullptr;

double* makeBinFreqs()
{
  double* bin_freq = new double[kFourierBlocksize/2 + 1];
  bin_freq[0] = 0;
  for (int i = 1; i < kFourierBlocksize / 2 + 1; i++)
    bin_freq[i] = bin_freq[i-1] + kBinWidth;
  return bin_freq;
}

void loadOrCreateWisdom()
{
  std::string wisdom_path = getAndEnsureConfigDir() + "1d_blocksize" +
                            std::to_string(kFourierBlocksize)+"_real_to_complex.fftw_wisdom";
  if (!fftw_import_wisdom_from_filename(wisdom_path.c_str()))
  {
    printf("No wisdom file found. Will now let FFTW practice a bit to learn...\n");
    double* in = fftw_alloc_real(kFourierBlocksize);
    fftw_complex* out = fftw_alloc_complex(kFourierBlocksize / 2 + 1);
    fftw_plan fft_plan = fftw_plan_dft_r2c_1d(kFourierBlocksize, in, out, FFTW_PATIENT | FFTW_DESTROY_INPUT);
    printf("Wisdom acquired. Now writing...");
    if (!fftw_export_wisdom_to_filename(wisdom_path.c_str()))
      printf("unable to write to %s\n", wisdom_path.c_str());
    else
      printf("done.\n");
    fftw_free(in);
    fftw_free(out);
    fftw_destroy_plan(fft_plan);
  }
}

EasyFourier::EasyFourier() : bin_freq_(makeBinFreqs())
{
  // (have to load / create wisdom BEFORE creating FourierWorkers, or else the
  //  workers won't benefit from the wisdom).
  loadOrCreateWisdom();

  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0)
    num_threads = 1;
  if (num_threads > 32) // let's not get too crazy
    num_threads = 32;
  for (int i=0; i<num_threads; i++)
    workers_.emplace_back(std::make_unique<FourierWorker>());
}

EasyFourier::~EasyFourier()
{
  delete[] bin_freq_;
}

FourierLease EasyFourier::borrowWorker()
{
  int id = pickAndLockWorker();
  return FourierLease(workers_[id]->in, workers_[id]->out,
                      workers_[id]->fft_plan, id, this);
}

double EasyFourier::freqOfBin(int index) const { return bin_freq_[index]; }

void EasyFourier::printEqualizer(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
    lease.in[i] = samples[i * kNumChannels];
  lease.runFFT();

  constexpr int kMaxHeight = 50;
  const int columns = kFourierBlocksize / 2 + 1; // including \n char at end
  char bars[columns * kMaxHeight + 1];
  for (int height = kMaxHeight; height > 0; height--)
  {
    int y = kMaxHeight - height;
    for (int x = 0; x < kFourierBlocksize / 2; x++)
      if (lease.out[x+1][0]*lease.out[x+1][0] + lease.out[x+1][1]*lease.out[x+1][1] > 2*height)
        bars[columns * y + x] = '0';
      else
        bars[columns * y + x] = ' ';
    bars[columns * y + kFourierBlocksize / 2] = '\n';
  }
  printf("\e[1;1H\e[2J");
  bars[columns * kMaxHeight] = 0;
  printf("%s", bars);
}

void EasyFourier::printMaxBucket(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
    lease.in[i] = samples[i * kNumChannels];
  lease.runFFT();

  double max = 0;
  int max_ind = 0;
  for (int i = 0; i < kFourierBlocksize / 2 + 1; i++)
  {
    double power = lease.out[i][0]*lease.out[i][0] + lease.out[i][1]*lease.out[i][1];
    if (power > max)
    {
      max = power;
      max_ind = i;
    }
  }
  float fraction = (float)max_ind / (float)(kFourierBlocksize / 2 + 1);
  printf("%g: %g\n", fraction * kNyquist, max);
}

int EasyFourier::pickAndLockWorker()
{
  for (int i=0; i<workers_.size(); i++)
    if (workers_[i]->mutex.try_lock())
      return i;

  int random_await_index = rand() % workers_.size();
  workers_[random_await_index]->mutex.lock();
  return random_await_index;
}

void EasyFourier::releaseWorker(int id)
{
  workers_[id]->mutex.unlock();
}

EasyFourier::FourierWorker::FourierWorker()
  : in(fftw_alloc_real(kFourierBlocksize)),
    out(fftw_alloc_complex(kFourierBlocksize / 2 + 1)),
    fft_plan(fftw_plan_dft_r2c_1d(kFourierBlocksize, in, out, FFTW_PATIENT | FFTW_DESTROY_INPUT))
{}

EasyFourier::FourierWorker::~FourierWorker()
{
  fftw_free(in);
  fftw_free(out);
  fftw_destroy_plan(fft_plan);
}

FourierLease::FourierLease(double* input, fftw_complex* output, fftw_plan plan,
                           int i, EasyFourier* p)
  : in(input), out(output), fft_plan(plan), id(i), parent(p) {}

void FourierLease::runFFT()
{
  fftw_execute(fft_plan); // (fft_plan is assumed to point at in and out).
}

FourierLease::~FourierLease() { parent->releaseWorker(id); }
