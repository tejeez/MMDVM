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

    // Polyphase filter taps
    std::vector<float> m_taps;
    // Downconversion sine table
    std::vector<std::complex<float>> m_ddc_sine;
    // Upconversion sine table
    std::vector<std::complex<float>> m_duc_sine;
};
