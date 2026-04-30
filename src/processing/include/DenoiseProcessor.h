#ifndef PROCESSING_DENOISE_PROCESSOR_H
#define PROCESSING_DENOISE_PROCESSOR_H

#include "radar_processor_base.h"
#include "kissfft.hh"
#include "../../core/Config.h"
#include <vector>
#include <cmath>
#include <memory>

namespace radar {

    class DenoiseProcessor : public Processor {
    public:
        DenoiseProcessor(const DenoiseConfig& config);
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        AudioFrame applyBandpassFilter(const AudioFrame& frame);
        std::vector<double> applyALE(const std::vector<double>& x, int stage);
        void applyFFT(std::vector<std::complex<double>>& data);
        void applyIFFT(std::vector<std::complex<double>>& data);

        DenoiseConfig m_config;
        std::unique_ptr<kissfft<double>> m_fft;
        std::unique_ptr<kissfft<double>> m_ifft;
    };

}

#endif
