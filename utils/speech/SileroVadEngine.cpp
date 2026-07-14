#include "SileroVadEngine.h"

#include <onnxruntime_cxx_api.h>

#include <QtEndian>

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace
{
//模型每个窗口需要拼接前一窗口末尾的上下文采样。
constexpr int kContextSamples = 64;
//Silero VAD 的循环状态形状为 [2, 1, 128]。
constexpr int kStateSize = 2 * 1 * 128;
} //namespace

//将 ONNX Runtime 的对象和模型状态隐藏在实现类中，避免头文件暴露依赖。
struct SileroVadEngine::Impl
{
    Impl()
        : environment(ORT_LOGGING_LEVEL_WARNING, "ZcChat2SileroVad"),
          memoryInfo(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,
                                                OrtMemTypeCPU)),
          state(kStateSize, 0.0f), context(kContextSamples, 0.0f)
    {
        //VAD 是实时单路推理，限制线程数避免占用过多 CPU。
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetInterOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(
            GraphOptimizationLevel::ORT_ENABLE_ALL);
    }

    Ort::Env environment;
    Ort::SessionOptions sessionOptions;
    Ort::MemoryInfo memoryInfo;
    std::unique_ptr<Ort::Session> session;
    std::vector<float> state;
    std::vector<float> context;
    std::array<int64_t, 2> inputDimensions{1,
                                           SileroVadEngine::WindowSamples +
                                               kContextSamples};
    std::array<int64_t, 3> stateDimensions{2, 1, 128};
};

SileroVadEngine::SileroVadEngine() : m_impl(std::make_unique<Impl>())
{
}

SileroVadEngine::~SileroVadEngine() = default;

/*加载 Silero VAD 模型*/
bool SileroVadEngine::initialize(const QByteArray &modelData,
                                 QString *errorMessage)
{
    if (modelData.isEmpty())
    {
        if (errorMessage)
            *errorMessage = QStringLiteral("Silero VAD 模型为空");
        return false;
    }

    try
    {
        m_impl->session = std::make_unique<Ort::Session>(
            m_impl->environment, modelData.constData(),
            static_cast<size_t>(modelData.size()), m_impl->sessionOptions);
        reset();
        return true;
    }
    catch (const Ort::Exception &exception)
    {
        m_impl->session.reset();
        if (errorMessage)
            *errorMessage = QString::fromUtf8(exception.what());
        return false;
    }
}

/*检查模型是否已加载*/
bool SileroVadEngine::isReady() const
{
    return m_impl->session != nullptr;
}

/*推理一个 512 采样的 PCM 音频窗口*/
float SileroVadEngine::processPcm16(const QByteArray &pcmChunk,
                                    QString *errorMessage)
{
    if (!isReady() || pcmChunk.size() != WindowBytes)
    {
        if (errorMessage)
            *errorMessage = isReady()
                                ? QStringLiteral("Silero VAD 音频块长度无效")
                                : QStringLiteral("Silero VAD 尚未初始化");
        return -1.0f;
    }

    //将 16-bit little-endian PCM 转为模型需要的浮点采样。
    std::vector<float> input(WindowSamples + kContextSamples, 0.0f);
    std::copy(m_impl->context.begin(), m_impl->context.end(), input.begin());
    for (int index = 0; index < WindowSamples; ++index)
    {
        qint16 rawSample = 0;
        std::memcpy(&rawSample,
                    pcmChunk.constData() + index * static_cast<int>(sizeof(qint16)),
                    sizeof(rawSample));
        const qint16 sample = qFromLittleEndian(rawSample);
        input.at(kContextSamples + index) =
            static_cast<float>(sample) / 32768.0f;
    }

    int64_t sampleRate = SampleRate;
    try
    {
        //模型输入依次为音频、循环状态和采样率。
        std::vector<Ort::Value> inputs;
        inputs.emplace_back(Ort::Value::CreateTensor<float>(
            m_impl->memoryInfo, input.data(), input.size(),
            m_impl->inputDimensions.data(), m_impl->inputDimensions.size()));
        inputs.emplace_back(Ort::Value::CreateTensor<float>(
            m_impl->memoryInfo, m_impl->state.data(), m_impl->state.size(),
            m_impl->stateDimensions.data(), m_impl->stateDimensions.size()));
        inputs.emplace_back(Ort::Value::CreateTensor<int64_t>(
            m_impl->memoryInfo, &sampleRate, 1, nullptr, 0));

        constexpr std::array<const char *, 3> inputNames{"input", "state",
                                                         "sr"};
        constexpr std::array<const char *, 2> outputNames{"output", "stateN"};
        std::vector<Ort::Value> outputs = m_impl->session->Run(
            Ort::RunOptions{nullptr}, inputNames.data(), inputs.data(),
            inputs.size(), outputNames.data(), outputNames.size());

        const float probability = outputs.at(0).GetTensorData<float>()[0];
        const float *nextState = outputs.at(1).GetTensorData<float>();
        //保存下一窗口需要的状态和上下文。
        std::copy(nextState, nextState + kStateSize, m_impl->state.begin());
        std::copy(input.end() - kContextSamples, input.end(),
                  m_impl->context.begin());
        return std::clamp(probability, 0.0f, 1.0f);
    }
    catch (const Ort::Exception &exception)
    {
        if (errorMessage)
            *errorMessage = QString::fromUtf8(exception.what());
        reset();
        return -1.0f;
    }
}

/*重置模型上下文*/
void SileroVadEngine::reset()
{
    std::fill(m_impl->state.begin(), m_impl->state.end(), 0.0f);
    std::fill(m_impl->context.begin(), m_impl->context.end(), 0.0f);
}
