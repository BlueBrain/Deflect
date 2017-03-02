YUV & Chroma Subsampling
============

This document provides general information about the YUV decoding and
chrominance subsampling support introduced in Deflect 0.13.

## Introduction

The JPEG algorithm must convert RGB input images to YUV prior to performing the
actual compression. The inverse transform is applied when decoding.

The rendering process can be made faster by skipping the YUV -> RGB step and
displaying the YUV images directly through an OpenGL shader.

## Chroma Subsampling

In addition to JPEG compression, the image size can be reduced by subsampling
the chrominance channels in the YUV color space. This technique is widely used
for movie encoding (most codecs use YUV420 by default). It has limited impact
on visual quality when applied to "natural" images, although it can be more
detrimental to artifical images such as computer interfaces.

Supported chrominance subsampling modes in Deflect are:

* YUV444 - No sub-sampling (default, image size: 100%)
* YUV422 - 50% vertical sub-sampling (image size: 75%)
* YUV420 - 50% vertical + horizontal sub-sampling (image size: 50%)

## Results

Chroma subsampling test results obtained with a 3840x1200[px] desktop stream.

| JPEG Quality  | YUV444 vs YUV420 | JPEG size reduction (%) |
| ------------- | ---------------- | -----------------------:|
| 20            | 320 / 250 KB     | 22 %                    |
| 50            | 450 / 370 KB     | 18 %                    |
| 75            | 590 / 490 KB     | 17 %                    |
| 90            | 850 / 700 KB     | 17 %                    |
| 100           | 2000 / 1500 KB   | 25 %                    |

We observe modest gains of network bandwidth using YUV420 subsampling (17% at
quality 75), but further performance is gained on the server side during
the transfer of the decompressed YUV image to GPU memory (50% less data).
