#include "easy_fourier.h"

#include <thread>

#include "config_io.h"
#include "constants.h"

EasyFourier* g_fourier = nullptr;

double* makeBinFreqs()
{
  double* bin_freq = new double[kNumFourierBins];
  bin_freq[0] = 0;
  for (int i = 1; i < kNumFourierBins; i++)
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
    fftw_complex* out = fftw_alloc_complex(kNumFourierBins);
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

int EasyFourier::binContainingFreq(double freq) const
{
  int lower_bound = (int)(freq / kBinWidth);
  if (lower_bound >= kNumFourierBins - 1)
    return kNumFourierBins - 1;
  if (lower_bound == 0)
    return 1;
  double diff_below = freq - bin_freq_[lower_bound];
  double diff_above = bin_freq_[lower_bound+1] - freq;
  if (diff_below < diff_above)
    return lower_bound;
  return lower_bound+1;
}

void EasyFourier::printOctavesAlreadyFreq(fftw_complex* powers) const
{
  double o1 = powers[1][0];
  double o2 = powers[2][0]+powers[3][0];
  double o3 = powers[4][0]+powers[5][0]+powers[6][0]+powers[7][0];
  double o4 = 0; for (int i=0;i<8;i++) o4+=powers[8+i][0];
  double o5 = 0; for (int i=0;i<16;i++) o5+=powers[16+i][0];
  double o6 = 0; for (int i=0;i<32;i++) o6+=powers[32+i][0];
  double o7 = 0; for (int i=0;i<64;i++) o7+=powers[64+i][0];

  if (o5 > 1000 || o6 > 200 || o7 > 100)
    printf("%f\t%f\t%f\t%f\t%f\t%f\t%f\n", o1,o2,o3,o4,o5,o6,o7);
}

void EasyFourier::printOctavePowers(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (g_num_channels == 2)
      lease.in[i] = (samples[i*g_num_channels] + samples[i*g_num_channels + 1]) / 2.0;
    else
      lease.in[i] = samples[i];
  }
  lease.runFFT();

  for (int x = 0; x < kNumFourierBins; x++)
    lease.out[x][0]=lease.out[x][0]*lease.out[x][0] + lease.out[x][1]*lease.out[x][1];
  printOctavesAlreadyFreq(lease.out);
}

double powerIfInBounds(fftw_complex* bins, int i)
{
  if (i < 1 || i > kFourierBlocksize/2)
    return 0;
  return bins[i][0];
}
bool isSpike(fftw_complex* p, int i)
{
  return powerIfInBounds(p, i-1) < p[i][0] && powerIfInBounds(p, i-2) < p[i][0] &&
         powerIfInBounds(p, i+1) < p[i][0] && powerIfInBounds(p, i+2) < p[i][0];
}
void EasyFourier::printTopTwoSpikes(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (g_num_channels == 2)
      lease.in[i] = (samples[i*g_num_channels] + samples[i*g_num_channels + 1]) / 2.0;
    else
      lease.in[i] = samples[i];
  }
  lease.runFFT();

  for (int x = 0; x < kNumFourierBins; x++)
    lease.out[x][0]=lease.out[x][0]*lease.out[x][0] + lease.out[x][1]*lease.out[x][1];

  int s1i = 0; int s2i = 0;
  double s1 = 0; double s2 = 0;
  for (int i=1; i<kNumFourierBins; i++)
  {
    if (lease.out[i][0] > s1 && isSpike(lease.out, i))
    {
      s2 = s1;
      s2i = s1i;
      s1 = lease.out[i][0];
      s1i = i;
    }
    else if (lease.out[i][0] > s2 && isSpike(lease.out, i))
    {
      s2 = lease.out[i][0];
      s2i = i;
    }
  }
  if (s2 > 30)
    printf("%d, %g,    %d, %g\n", s1i, s1, s2i, s2);
}

void EasyFourier::printOvertones(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (g_num_channels == 2)
      lease.in[i] = (samples[i*g_num_channels] + samples[i*g_num_channels + 1]) / 2.0;
    else
      lease.in[i] = samples[i];
  }
  lease.runFFT();

  for (int x = 0; x < kNumFourierBins; x++)
    lease.out[x][0]=lease.out[x][0]*lease.out[x][0] + lease.out[x][1]*lease.out[x][1];

  int s1i = 0;
  double s1 = 0;
  for (int i=1; i<kNumFourierBins; i++)
  {
    if (lease.out[i][0] > s1 && isSpike(lease.out, i))
    {
      s1 = lease.out[i][0];
      s1i = i;
    }
  }

  // We don't know exactly where in the bin our spike was: just doubling the
  // index might overshoot or undershoot. Instead, we should look at the entire
  // possible range.
  double min_freq = g_fourier->freqOfBin(s1i) - kBinWidth/2.0;
  double max_freq = g_fourier->freqOfBin(s1i) + kBinWidth/2.0;

  if (s1 > 1000)
  {
    // (looking at the 3rd and 5th overtones because that's what seems to show
    //  up for me when inhale-whistling near the mic)
    int min3 = g_fourier->binContainingFreq(min_freq * 3.0);
    int max3 = g_fourier->binContainingFreq(max_freq * 3.0);
    int min5 = g_fourier->binContainingFreq(min_freq * 5.0);
    int max5 = g_fourier->binContainingFreq(max_freq * 5.0);
    printf("%d, %g,     ", s1i, s1);
    for (int i = min3; i<= max3; i++)
      printf("%d, %g, ", i, lease.out[i][0] > 50 ? lease.out[i][0] : 0);
    printf("      ");
    for (int i = min5; i<= max5; i++)
      printf("%d, %g, ", i, lease.out[i][0] > 50 ? lease.out[i][0] : 0);
    printf("\n");
  }
}

void EasyFourier::printEqualizer(const float* samples)
{
  FourierLease lease = borrowWorker();
  for (int i=0; i<kFourierBlocksize; i++)
  {
    if (g_num_channels == 2)
      lease.in[i] = (samples[i*g_num_channels] + samples[i*g_num_channels + 1]) / 2.0;
    else
      lease.in[i] = samples[i];
  }
  lease.runFFT();

  for (int x = 0; x < kNumFourierBins; x++)
    lease.out[x][0]=lease.out[x][0]*lease.out[x][0] + lease.out[x][1]*lease.out[x][1];

  constexpr int kMaxHeight = 50;
  const int columns = kNumFourierBins; // including \n char at end
  char bars[columns * kMaxHeight + 1];
  for (int height = kMaxHeight; height > 0; height--)
  {
    int y = kMaxHeight - height;
    for (int x = 0; x < kFourierBlocksize / 2; x++)
      if (lease.out[x+1][0] > 2*height)
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
  {
    if (g_num_channels == 2)
      lease.in[i] = (samples[i*g_num_channels] + samples[i*g_num_channels + 1]) / 2.0;
    else
      lease.in[i] = samples[i];
  }
  lease.runFFT();

  double max = 0;
  int max_ind = 0;
  for (int i = 0; i < kNumFourierBins; i++)
  {
    double power = lease.out[i][0]*lease.out[i][0] + lease.out[i][1]*lease.out[i][1];
    if (power > max)
    {
      max = power;
      max_ind = i;
    }
  }
  float fraction = (float)max_ind / (float)(kNumFourierBins);
  printf("%g: %g\n", fraction * kNyquist, max);
}

int EasyFourier::pickAndLockWorker()
{
  while (true)
  {
    for (int i=0; i<workers_.size(); i++)
      if (workers_[i]->mutex.try_lock())
        return i;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void EasyFourier::releaseWorker(int id)
{
  workers_[id]->mutex.unlock();
}

EasyFourier::FourierWorker::FourierWorker()
  : in(fftw_alloc_real(kFourierBlocksize)),
    out(fftw_alloc_complex(kNumFourierBins)),
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
