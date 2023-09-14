#include "pch.h"
#include "ImageStacking.h"

void ImageStacking::ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator scale_est) {

    float rse, cse;
    for (int ch = 0; ch < tgt.Channels(); ++ch) {

        switch (scale_est) {
        case ScaleEstimator::median:
            rse = ref.Median(ch);
            cse = tgt.ComputeMedian(ch, true);//tgt.Median(ch);
            break;
        case ScaleEstimator::avgdev:
            rse = ref.AvgDev(ch);
            cse = tgt.ComputeAvgDev(ch, true);
            break;
        case ScaleEstimator::mad:
            rse = ref.MAD(ch);
            cse = tgt.ComputeMAD(ch, true);
            break;
        case ScaleEstimator::bwmv:
            rse = ref.BWMV(ch);
            cse = tgt.ComputeBWMV(ch, true);
            break;
        case ScaleEstimator::none:
            return;

        }

        float k = rse / cse;


        for (auto pixel = tgt.begin(ch); pixel != tgt.end(ch); ++pixel)
            *pixel = tgt.ClipPixel(k * (*pixel - tgt.Median(ch)) + ref.Median(ch));
    }
}

void ImageStacking::ScaleImageStack(ImageVector& img_stack, ScaleEstimator scale_est) {

    if (scale_est == ScaleEstimator::none)
        return;

    switch (scale_est) {
    case ScaleEstimator::median:
        img_stack[0].ComputeMedian();
        break;
    case ScaleEstimator::avgdev:
        img_stack[0].ComputeAvgDev();
        break;
    case ScaleEstimator::mad:
        img_stack[0].ComputeMAD();
        break;
    case ScaleEstimator::bwmv:
        img_stack[0].ComputeBWMV();
        break;
    case ScaleEstimator::none:
        return;
    }

    for (auto iter = img_stack.begin() + 1; iter != img_stack.end(); ++iter)
        ScaleImage(*img_stack.begin(), *iter, scale_est);

}


void ImageStacking::RemoveOutliers(std::vector<float>& pixstack, float l_limit, float u_limit) {

    for (auto it = pixstack.begin(); it != pixstack.end(); ++it)
        if (*it<l_limit || *it>u_limit) {
            pixstack.erase(it);
            it--;
        }
}

static float StandardDeviation(const std::vector<float>& pixelstack, float mean) {

    double d;
    double var = 0;

    for (const float& pixel : pixelstack) {
        d = pixel - mean;
        var += d * d;
    }

    return (float)sqrt(var / pixelstack.size());
}

float ImageStacking::Mean(const std::vector<float>& pixelstack) {

    float mean = 0;
    for (const float& pixel : pixelstack)
        mean += pixel;

    return mean / pixelstack.size();
}

float ImageStacking::Median(std::vector<float>& pixelstack) {

    std::nth_element(pixelstack.begin(), pixelstack.begin() + pixelstack.size() / 2, pixelstack.end());

    return pixelstack[pixelstack.size() / 2];
}

float ImageStacking::Min(const std::vector<float>& pixelstack) {

    float min = std::numeric_limits<float>::max();

    for (const float& pixel : pixelstack)
        if (pixel < min)
            min = pixel;

    return min;
}

float ImageStacking::StandardDeviation(const std::vector<float>& pixelstack) {

    float mean = 0;
    for (const float& pixel : pixelstack)
        mean += pixel;

    mean /= pixelstack.size();

    double d;
    double var = 0;
    for (const float& pixel : pixelstack) {
        d = pixel - mean;
        var += d * d;
    }

    return (float)sqrt(var / pixelstack.size());
}

void ImageStacking::SigmaClip(std::vector<float>& pixelstack, float l_sigma, float u_sigma) {

    std::sort(pixelstack.begin(), pixelstack.end());
    float old_stddev = 0;

    for (int iter = 0; iter < 5; iter++) {
        float median = pixelstack[pixelstack.size() / 2];
        float stddev = StandardDeviation(pixelstack);

        if (stddev == 0 || (old_stddev - stddev) == 0)  break;

        RemoveOutliers(pixelstack, median - l_sigma * stddev, median + u_sigma * stddev);

        old_stddev = stddev;
    }
}

void ImageStacking::WinsorizedSigmaClip(std::vector<float>& pixelstack, float l_sigma, float u_sigma) {

    int mid_point = pixelstack.size() / 2;

    std::sort(pixelstack.begin(), pixelstack.end());
    float old_stddev = 0;

    for (int iter = 0; iter < 5; iter++) {
        float median = pixelstack[mid_point];
        float stddev = StandardDeviation(pixelstack);

        if (stddev == 0 || (old_stddev - stddev) == 0) break;

        float u_thresh = median + u_sigma * stddev;
        for (int upper = mid_point; upper < pixelstack.size(); ++upper)
            if (pixelstack[upper] > u_thresh) pixelstack[upper] = pixelstack[upper - 1];


        float l_thresh = median - l_sigma * stddev;
        for (int lower = mid_point; lower >= 0; lower--)
            if (pixelstack[lower] < l_thresh) pixelstack[lower] = pixelstack[lower + 1];

        old_stddev = stddev;
    }
}


void ImageStacking::IntegrateImages(ImageVector& img_stack, Image32& output) {
    using enum Integration;
    using enum Rejection;

    ScaleImageStack(img_stack, m_scale_est);

    output = Image32(img_stack[0].Rows(), img_stack[0].Cols());

    std::vector<float> pixelstack(img_stack.size());

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < output.Total(); ++el) {

        if (pixelstack.size() != pixelstack.capacity())
            pixelstack.resize(pixelstack.capacity());

        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = img_stack[i][el];

        switch (m_rejection) {
        case none:
            break;
        case sigma_clip:
            SigmaClip(pixelstack, m_l_sigma, m_u_sigma);
            break;
        case winsorized_sigma_clip:
            WinsorizedSigmaClip(pixelstack, 2, 3);
            break;
        default:
            break;
        }

        switch (m_integration) {
        case average:
            output[el] = Mean(pixelstack);
            break;
        case median:
            output[el] = Median(pixelstack);

        }
    }
}
