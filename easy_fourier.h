#ifndef CLICKITONGUE_EASY_FOURIER_H_
#define CLICKITONGUE_EASY_FOURIER_H_

#include <memory>
#include <mutex>
#include <vector>

#include <fftw3.h>

// Safe, orderly multithreaded invocations of FFTW's 1d real-to-complex FFT.

// Usage:
// 1) Get a FourierLease from EasyFourier using borrowWorker()
// 2) Load your input into lease.in (an array of blocksize reals)
// 3) Call lease.runFFT()
// 4) Read from lease.out (an array of blocksize/2 + 1 complexes)
// 5) Let the lease go out of scope to release its worker.

struct FourierLease;

class EasyFourier
{
public:
  EasyFourier() = delete;
  explicit EasyFourier(int blocksize);
  ~EasyFourier();

  FourierLease borrowWorker();

  // Returns the frequency center of the provided bin index.
  double freqOfBin(int index) const;

  // In Hz, the difference between two adjacent bin centers.
  double binWidth() const;
  // 0.5 * binWidth
  double halfWidth() const;

  // length of samples should be blocksize_.
  void printEqualizer(const float* samples);

  // length of freq_buckets should be blocksize_ / 2 + 1.
  // (i.e. the output of a real-to-complex FFT with length blocksize_ input)
  void printEqualizerAlreadyFreq(fftw_complex* freq_buckets) const;

  // prints the frequency bucket with the most energy, and its energy.
  // length of samples should be blocksize_.
  void printMaxBucket(const float* samples);

  int blocksize() const;

private:
  friend struct FourierLease;
  void releaseWorker(int id);

  struct FourierWorker
  {
  public:
    explicit FourierWorker(int blocksize);
    ~FourierWorker();
    double* in;
    fftw_complex* out;
    fftw_plan fft_plan;
    std::mutex mutex;
  };
  int pickAndLockWorker();
  std::vector<std::unique_ptr<FourierWorker>> workers_;

  int blocksize_;
  double bin_width_;
  double half_width_;
  const double* bin_freq_;
};

struct FourierLease
{
public:
  FourierLease(double* input, fftw_complex* output, fftw_plan plan,
               int i, EasyFourier* p);
  ~FourierLease();

  void runFFT(); // reads from this struct's 'in', writes to its 'out'

  // length: parent->blocksize elements
  double* in;
  // length: parent->blocksize / 2 + 1 elements
  fftw_complex* out;

private:
  fftw_plan fft_plan;
  int id;
  EasyFourier* parent;
};

extern EasyFourier* g_fourier;

#endif // CLICKITONGUE_EASY_FOURIER_H_
