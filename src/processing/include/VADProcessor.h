#ifndef PROCESSING_VAD_PROCESSOR_H
#define PROCESSING_VAD_PROCESSOR_H

#include "radar_processor_base.h"

namespace radar {

    class VADProcessor : public Processor {
    public:
        explicit VADProcessor(double threshold = 800.0);
        Result<ProcessedData> process(const AudioFrame& frame) override;

    private:
        double m_threshold;
        bool detectVoice(const AudioFrame& frame);
    };

} // namespace radar

#endif // PROCESSING_VAD_PROCESSOR_H