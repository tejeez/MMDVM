/*
 * Digital up and down conversion with fractional sample rate conversion
 *
 */

#include <complex>
#include <functional>
#include <vector>

class FDUDC {
public:
    FDUDC(
        unsigned resampNum,
        unsigned resampDen,
        int      rxIfNum,
        unsigned rxIfDen,
        int      txIfNum,
        unsigned txIfDen
    );
    void process(
        std::vector<std::complex<float>> &buffer,
        std::function<std::complex<float>(std::complex<float>)> process_sample);
private:
    // Numerator of sample rate ratio
    // This determines the interpolation factor for DDC
    // and decimation factor for DUC.
    unsigned m_resampNum;
    // Denominator of sample rate ratio.
    // This determines the decimation factor for DDC
    // and interpolation factor for DUC.
    unsigned m_resampDen;
    // RX I/Q intermediate frequency,
    // i.e. frequency that will be converted to 0 in DDC,
    // as a fraction of radio sample rate.
    int      m_rxIfNum;
    unsigned m_rxIfDen;
    // TX I/Q intermediate frequency
    // as a fraction of radio sample rate.
    int      m_txIfNum;
    unsigned m_txIfDen;

    // Polyphase filter phase
    unsigned m_p;
    // Index to m_in and m_out
    unsigned m_i;
    // Index to m_ddc_sine
    unsigned m_ddc_i;
    // Index to m_duc_sine
    unsigned m_duc_i;
    // Polyphase filter taps
    std::vector<float> m_taps;
    // Input buffer for DDC resampler
    std::vector<std::complex<float>> m_in;
    // Output buffer for DUC resampler
    std::vector<std::complex<float>> m_out;
    // Downconversion sine table
    std::vector<std::complex<float>> m_ddc_sine;
    // Upconversion sine table
    std::vector<std::complex<float>> m_duc_sine;
};
