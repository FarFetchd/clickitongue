#include "easy_fourier.h"

#include <cmath>
#include <thread>

#include "constants.h"

EasyFourier* g_fourier = nullptr;

double* makeBinFreqs(int blocksize, double bin_width)
{
  double* bin_freq = new double[blocksize/2 + 1];
  bin_freq[0] = 0;
  for (int i = 1; i < blocksize / 2 + 1; i++)
    bin_freq[i] = bin_freq[i-1] + bin_width;
  return bin_freq;
}

EasyFourier::EasyFourier(int blocksize)
  : blocksize_(blocksize), bin_width_(kNyquist / ((double)(blocksize_ / 2 + 1))),
    half_width_(bin_width_ / 2.0), bin_freq_(makeBinFreqs(blocksize_, bin_width_))
{
  unsigned int num_threads = std::thread::hardware_concurrency();
  if (num_threads == 0)
    num_threads = 1;
  if (num_threads > 32) // let's not get too crazy
    num_threads = 32;
  for (int i=0; i<num_threads; i++)
    workers_.emplace_back(std::make_unique<FourierWorker>(blocksize_));
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
double EasyFourier::binWidth() const { return bin_width_; }
double EasyFourier::halfWidth() const { return half_width_; }

void EasyFourier::printEqualizer(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<blocksize_; i++)
    lease.in[i] = samples[i * kNumChannels];
  lease.runFFT();

  printEqualizerAlreadyFreq(lease.out);
}

void EasyFourier::printEqualizerAlreadyFreq(fftw_complex* freq_buckets) const
{
  constexpr int kMaxHeight = 50;
  const int columns = blocksize_ / 2 + 1; // including \n char at end
  char bars[columns * kMaxHeight + 1];
  for (int height = kMaxHeight; height > 0; height--)
  {
    int y = kMaxHeight - height;
    for (int x = 0; x < blocksize_ / 2; x++)
      if (fabs(freq_buckets[x][0]) > height)
        bars[columns * y + x] = '0';
      else
        bars[columns * y + x] = ' ';
    bars[columns * y + blocksize_ / 2] = '\n';
  }
  printf("\e[1;1H\e[2J");
  bars[columns * kMaxHeight] = 0;
  printf("%s", bars);
}

void EasyFourier::printMaxBucket(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<blocksize_; i++)
    lease.in[i] = samples[i * kNumChannels];
  lease.runFFT();

  double max = 0;
  int max_ind = 0;
  for (int i = 0; i < blocksize_ / 2 + 1; i++)
  {
    if (fabs(lease.out[i][0]) > max)
    {
      max = fabs(lease.out[i][0]);
      max_ind = i;
    }
  }
  float fraction = (float)max_ind / (float)(blocksize_ / 2 + 1);
  printf("%g: %g\n", fraction * kNyquist, max);
}

int EasyFourier::blocksize() const { return blocksize_; }

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

EasyFourier::FourierWorker::FourierWorker(int blocksize)
  : in(fftw_alloc_real(blocksize)),
    out(fftw_alloc_complex(blocksize / 2 + 1)),
    fft_plan(fftw_plan_dft_r2c_1d(blocksize, in, out, FFTW_ESTIMATE | FFTW_DESTROY_INPUT)) {} // TODO wisdom

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
