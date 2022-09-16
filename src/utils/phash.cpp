#include "phash.h"

#include <cmath>
#include <stdexcept>

void dct_axis_0(const QImage &img,
                       size_t img_size,
                       std::vector<double> &dct0);

void dct_axis_1(const std::vector<double> &dct0,
                       size_t img_size,
                       std::vector<double> &dct);

QByteArray hash_from_dct(const std::vector<double> &dct,
                          size_t img_size,
                          size_t hash_size);

QByteArray phash(QImage &image, int img_size) {

    std::vector<double> dct0, dct1;
    dct_axis_0(image, img_size, dct0);
    dct_axis_1(dct0, img_size, dct1);

    // 4 is highfreq_factor
    return hash_from_dct(dct1, img_size, img_size / 4);
}

void dct_axis_0(const QImage &img,
                      size_t img_size,
                      std::vector<double> &dct0) {

    dct0.resize(img_size * img_size, 0.0);
    std::vector<double> row;
    row.resize(img_size);
    size_t i, n, k;
    double factor = M_PI / (double)img_size;
    for (i = 0; i < img_size; ++ i) {
        for (n = 0; n < img_size; ++ n) {
            QColor pixel( img.pixel( n, i ) );
            row[n] = pixel.green();
        }
        for (k = 0; k < n; ++ k) {
            double & y = dct0[img_size * k + i];
            for (n = 0; n < img_size; ++ n) {
                y += row[n] * std::cos(k * (n + 0.5) * factor);
            }
            y *= 2;
        }
    }
}

void dct_axis_1(const std::vector<double> &dct0,
                      size_t img_size,
                      std::vector<double> &dct) {
    size_t i, n, k;
    double factor = M_PI / (double)img_size;
    dct.resize(img_size * img_size, 0.0);
    for (i = 0; i < img_size; ++ i) {
        for (k = 0; k < img_size; ++ k) {
            double & y = dct[img_size * i + k];
            for (n = 0; n < img_size; ++n) {
                y += dct0[img_size * i + n] * std::cos(k * (n + 0.5) * factor);
            }
            y *= 2;
        }
    }
}

QByteArray hash_from_dct(const std::vector<double> &dct,
                         size_t img_size,
                         size_t hash_size ) {
    std::vector<double> lowfreq;
    lowfreq.resize(hash_size * hash_size);
    size_t i, n;
    for (i = 0; i < hash_size; ++ i) {
        for (n = 0; n < hash_size; ++ n) {
            lowfreq[hash_size * i + n] = dct[img_size * i + n];
        }
    }
    std::vector<double> lowfreq_sorted = lowfreq;
    std::sort(lowfreq_sorted.begin(), lowfreq_sorted.end());
    size_t mid = (hash_size * hash_size) >> 1;
    double median = hash_size & 1 ?
        lowfreq_sorted[mid]
    :
        (lowfreq_sorted[mid - 1] + lowfreq_sorted[mid]) / 2.0
    ;

    QByteArray hash;

    for (i = 0; i < hash_size; ++ i) {
        for (n = 0; n < hash_size; ++ n) {
            hash.append(lowfreq[hash_size * i + n] > median);
        }
    }

    return hash;
}
