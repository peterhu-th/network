#ifndef PROCESSING_DENOISE_PROCESSOR_H
#define PROCESSING_DENOISE_PROCESSOR_H

#include "radar_processor_base.h"
#include "kissfft.hh"
#include <vector>
#include <cmath>
#include <memory>

namespace radar {

    class DenoiseProcessor : public Processor {
    public:
        DenoiseProcessor(double lowCutoff = 3000.0, double highCutoff = 17000.0, int sampleRate = 44100);
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        AudioFrame applyBandpassFilter(const AudioFrame& frame);
        std::vector<double> applyALE(const std::vector<double>& x, double mu = 0.001, int M = 16, int delta = 3);
        void applyFFT(std::vector<std::complex<double>>& data);
        void applyIFFT(std::vector<std::complex<double>>& data);

        double m_lowCutoff;
        double m_highCutoff;
        int m_sampleRate;
        int m_fftSize;
        std::unique_ptr<kissfft<double>> m_fft;
        std::unique_ptr<kissfft<double>> m_ifft;
    };

} // namespace radar

#endif // PROCESSING_DENOISE_PROCESSOR_H