#include "../include/AudioProcessingService.h"
#include "../include/DenoiseProcessor.h"
#include "../include/ProcessorFactory.h"
#include "../../core/types.h"
#include <QCoreApplication>
#include <QFile>
#include <QDebug>
#include <QDataStream>
#include <QDateTime>
#include <QIODevice>
#include <cmath>

using namespace radar;

struct WavHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t fileSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = 44100;
    uint32_t byteRate = 0;
    uint16_t blockAlign = 0;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};

AudioFrame loadWavFile(const QString& filePath) {
    AudioFrame frame;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << filePath;
        return frame;
    }

    WavHeader header;
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.readRawData(header.riff, 4);
    stream >> header.fileSize;
    stream.readRawData(header.wave, 4);
    stream.readRawData(header.fmt, 4);
    stream >> header.fmtSize;
    stream >> header.audioFormat;
    stream >> header.numChannels;
    stream >> header.sampleRate;
    stream >> header.byteRate;
    stream >> header.blockAlign;
    stream >> header.bitsPerSample;
    stream.readRawData(header.data, 4);
    stream >> header.dataSize;

    if (header.audioFormat != 1) {
        qDebug() << "仅支持 PCM 格式";
        return frame;
    }

    QByteArray rawData = file.readAll();
    file.close();

    frame.data = rawData;
    frame.sampleRate = header.sampleRate;
    frame.channels = header.numChannels;
    frame.sampleSize = header.bitsPerSample;
    frame.timestamp = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "WAV 文件信息:";
    qDebug() << "  采样率:" << frame.sampleRate;
    qDebug() << "  声道数:" << frame.channels;
    qDebug() << "  采样位数:" << frame.sampleSize;
    qDebug() << "  数据大小:" << frame.data.size() << "字节";
    qDebug() << "  样本数量:" << frame.data.size() / (frame.sampleSize / 8);

    return frame;
}

void saveWavFile(const QString& filePath, const AudioFrame& frame) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法保存文件:" << filePath;
        return;
    }

    WavHeader header;
    header.numChannels = frame.channels;
    header.sampleRate = frame.sampleRate;
    header.bitsPerSample = frame.sampleSize;
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.dataSize = frame.data.size();
    header.fileSize = 36 + header.dataSize;

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData(header.riff, 4);
    stream << header.fileSize;
    stream.writeRawData(header.wave, 4);
    stream.writeRawData(header.fmt, 4);
    stream << header.fmtSize;
    stream << header.audioFormat;
    stream << header.numChannels;
    stream << header.sampleRate;
    stream << header.byteRate;
    stream << header.blockAlign;
    stream << header.bitsPerSample;
    stream.writeRawData(header.data, 4);
    stream << header.dataSize;

    file.write(frame.data);
    file.close();

    qDebug() << "已保存:" << filePath;
}

void analyzeAudio(const QByteArray& data, int bitsPerSample) {
    const int16_t* samples = reinterpret_cast<const int16_t*>(data.constData());
    int numSamples = data.size() / sizeof(int16_t);

    double sum = 0.0;
    double maxVal = 0.0;
    int impulseCount = 0;
    double threshold = 0.0;

    for (int i = 0; i < numSamples; ++i) {
        double absVal = std::abs(static_cast<double>(samples[i]));
        sum += absVal;
        maxVal = std::max(maxVal, absVal);
    }

    double mean = sum / numSamples;

    double sumSq = 0.0;
    for (int i = 0; i < numSamples; ++i) {
        double diff = std::abs(static_cast<double>(samples[i])) - mean;
        sumSq += diff * diff;
    }
    double stdDev = std::sqrt(sumSq / numSamples);

    threshold = mean + 4.0 * stdDev;

    for (int i = 0; i < numSamples; ++i) {
        if (std::abs(static_cast<double>(samples[i])) > threshold) {
            impulseCount++;
        }
    }

    qDebug() << "\n音频分析结果:";
    qDebug() << "  样本数量:" << numSamples;
    qDebug() << "  均值:" << mean;
    qDebug() << "  标准差:" << stdDev;
    qDebug() << "  最大值:" << maxVal;
    qDebug() << "  4倍标准差阈值:" << threshold;
    qDebug() << "  检测到的脉冲数:" << impulseCount;
    qDebug() << "  脉冲比例:" << QString::number(100.0 * impulseCount / numSamples, 'f', 2) << "%";
}

void testImpulseNoiseRemoval(const QString& inputFile, double lowCutoff, double highCutoff,
                             int windowSize, double thresholdFactor) {
    qDebug() << "\n========== 测试参数 ==========";
    qDebug() << "  输入文件:" << inputFile;
    qDebug() << "  带通滤波:" << lowCutoff << "-" << highCutoff << "Hz";
    qDebug() << "  脉冲检测窗口:" << windowSize;
    qDebug() << "  脉冲阈值因子:" << thresholdFactor;

    AudioFrame inputFrame = loadWavFile(inputFile);
    if (inputFrame.data.isEmpty()) {
        qDebug() << "加载文件失败";
        return;
    }

    qDebug() << "\n--- 处理前分析 ---";
    analyzeAudio(inputFrame.data, inputFrame.sampleSize);

    DenoiseProcessor processor(lowCutoff, highCutoff, inputFrame.sampleRate);

    auto result = processor.process(inputFrame);
    if (result.isOk()) {
        qDebug() << "\n--- 处理后分析 ---";
        analyzeAudio(result.value().originalFrame.data, inputFrame.sampleSize);

        QString outputFile = inputFile;
        qsrand(QDateTime::currentMSecsSinceEpoch() % 100000);
        int randomNum = qrand() % 10000;
        outputFile.replace(".wav", "_filtered_" + QString::number(randomNum) + ".wav");
        saveWavFile(outputFile, result.value().originalFrame);
        
        qDebug() << "\n处理完成！";
        qDebug() << "  输出文件:" << outputFile;
    } else {
        qDebug() << "处理失败:" << result.errorMessage();
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    qDebug() << "===== 脉冲噪声去除测试工具 =====\n";

    QString inputFile = "test_input.wav";

    qDebug() << "截止频率测试 (2000-19000 Hz，仅带通滤波):";
    testImpulseNoiseRemoval(inputFile, 2000.0, 19000.0, 5, 999.0);

    qDebug() << "\n第2次测试 (2000-19000 Hz，带通滤波 + ALE):";
    testImpulseNoiseRemoval(inputFile, 2000.0, 19000.0, 5, 1.5);

    qDebug() << "\n\n参数调试建议:";
    qDebug() << "1. 如果脉冲没被去除 → 降低 thresholdFactor (如 2.0, 1.5)";
    qDebug() << "2. 如果好信号被去除 → 提高 thresholdFactor (如 3.0, 4.0)";
    qDebug() << "3. 如果脉冲宽度大 → 增大 windowSize (如 7, 9)";
    qDebug() << "4. 如果脉冲宽度小 → 减小 windowSize (如 3)";

    return 0;
}