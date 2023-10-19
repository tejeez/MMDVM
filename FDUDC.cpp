/*
 * Digital up and down conversion with fractional sample rate conversion
 *
 */

#include "FDUDC.h"

FDUDC::FDUDC(
    unsigned resampNum,
    unsigned resampDen,
    int      rxIfNum,
    unsigned rxIfDen,
    int      txIfNum,
    unsigned txIfDen
):
m_resampNum(resampNum),
m_resampDen(resampDen),
m_rxIfNum(rxIfNum),
m_rxIfDen(rxIfDen),
m_txIfNum(txIfNum),
m_txIfDen(txIfDen)
{
};

void FDUDC::process(
    std::vector<std::complex<float>> &buffer,
    std::function<std::complex<float>(std::complex<float>)> process_sample
)
{
    for (auto &s : buffer) {
        // TODO
        // To test the test
        s = process_sample(s);
    }
}

// To compile a test program and run it:
// g++ -o obj_linux/test_FDUDC FDUDC.cpp -Wall -Wextra -DTEST_FDUDC
// ../testsignal --format=cf32le --samples=1000000 | obj_linux/test_FDUDC > test_duc_output.raw
// Where ../testsignal is a frequency sweep generator from
// https://github.com/tejeez/spektri/
#if defined(TEST_FDUDC)
#include <iostream>
#include <fstream>

#define TEST_BLOCK_LEN 4

int main(void)
{

    auto ddc_out_file = new std::ofstream("test_ddc_output.raw");
    FDUDC dudc(4, 25, 1, 24, 1, 24);
    std::vector<std::complex<float>> buf(TEST_BLOCK_LEN);
    for (;;) {
        std::cin.read((char*)buf.data(), buf.size() * sizeof(std::complex<float>));
        if (std::cin.fail())
            break;
        dudc.process(buf, [ddc_out_file](std::complex<float> s) {
            ddc_out_file->write((char*)&s, sizeof(s));
            return s;
        });
        std::cout.write((const char*)buf.data(), buf.size() * sizeof(std::complex<float>));
    }
}
#endif
