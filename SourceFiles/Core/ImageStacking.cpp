#include "pch.h"
#include "ImageStacking.h"
#include "Drizzle.h"

ImageStacking::PixelRows::PixelRows(int num_imgs, int width, ImageStacking& is) : m_width(width), m_num_imgs(num_imgs), m_is(&is) {
    m_size = num_imgs * width;
    m_pixels = std::vector<float>(m_size);
}

void ImageStacking::PixelRows::fill(const ImagePoint& start_point) {

//#pragma omp parallel for num_threads(m_max_threads)
    for (int i = 0; i < m_is->m_imgfile_vector.size(); ++i) {
        //get rid of file type?
        switch (m_is->m_imgfile_vector[i]->type()) {
        case ImageFile::Type::FITS:
            dynamic_cast<FITS*>(m_is->m_imgfile_vector[i].get())->readRow_toFloat(&(*this)(0, i), start_point.y(), start_point.channel());
            break;
        }
    }
}

void ImageStacking::PixelRows::fillPixelStack(std::vector<float>& pixelstack, int x, int ch) {

    using enum Normalization;

    if (pixelstack.size() != pixelstack.capacity())
        pixelstack.resize(pixelstack.capacity());

    switch (m_is->m_normalization) {
    case additive:

        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = (*this)(x, i) - m_is->m_le[i][ch] + m_is->m_le[0][ch];
        return;

    case multiplicative:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = (*this)(x, i) * (m_is->m_le[0][ch] / m_is->m_le[i][ch]);
        return;

    case additive_scaling:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = m_is->m_sf[i][ch] * ((*this)(x, i) - m_is->m_le[i][ch]) + m_is->m_le[0][ch];
        return;

    case multiplicative_scaling:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = m_is->m_sf[i][ch] * (*this)(x, i) * (m_is->m_le[0][ch] / m_is->m_le[i][ch]);
        return;

    case none:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = (*this)(x, i);
        return;
    }
}



float ImageStacking::mean(const std::vector<float>& pixelstack) {

    float mean = 0;

    for (float pixel : pixelstack)
        mean += pixel;

    return mean / pixelstack.size();
}

float ImageStacking::standardDeviation(const std::vector<float>& pixelstack) {

    float mean = 0;
    for (float pixel : pixelstack)
        mean += pixel;

    mean /= pixelstack.size();

    double d;
    double var = 0;
    for (float pixel : pixelstack) {
        d = pixel - mean;
        var += d * d;
    }

    return (float)sqrt(var / pixelstack.size());
}

float ImageStacking::median(std::vector<float>& pixelstack) {

    std::nth_element(pixelstack.begin(), pixelstack.begin() + pixelstack.size() / 2, pixelstack.end());

    return pixelstack[pixelstack.size() / 2];
}

float ImageStacking::min(const std::vector<float>& pixelstack) {

    float min = std::numeric_limits<float>::max();

    for (float pixel : pixelstack)
        if (pixel < min)
            min = pixel;

    return min;
}

float ImageStacking::max(const std::vector<float>& pixelstack) {

    float max = std::numeric_limits<float>::min();

    for (float pixel : pixelstack)
        if (pixel > max)
            max = pixel;

    return max;
}


static void MinMax(std::vector<float>& pixstack, int num_min, int num_max) {

    int mid = pixstack.size() / 2;

    if (num_min + num_max > pixstack.size() - 1) {
        if (num_min >= pixstack.size() || num_max >= pixstack.size())
            num_min = num_max = mid;

        if (num_max > mid)
            num_max = mid;
        if (num_min > mid)
            num_min = mid;
    }

    std::sort(pixstack.begin(), pixstack.end());

    //for (int l = 0, u = pixstack.size() - 1; l < pixstack.size() / 2; ++l, --u)
        //std::swap(pixstack[l], pixstack[u]);

    //for (int i = 0; i < num_min; ++i)
        //pixstack.erase(pixstack.begin());

    for (int i = 0; i < num_max; ++i)
        pixstack.pop_back();

    for (int l = 0, u = pixstack.size() - 1; l < u; ++l, --u)
        std::swap(pixstack[l], pixstack[u]);

    for (int i = 0; i < num_min; ++i)
        pixstack.pop_back();
}

void ImageStacking::sigmaClip(std::vector<float>& pixelstack, float l_sigma, float u_sigma) {

    std::sort(pixelstack.begin(), pixelstack.end());
    float old_stddev = 0;

    for (int iter = 0; iter < 5; ++iter) {
        float median = pixelstack[pixelstack.size() / 2];
        float stddev = standardDeviation(pixelstack);

        if (stddev == 0 || (old_stddev - stddev) == 0)  break;

        float l_limit = median - l_sigma * stddev;
        float u_limit = median + u_sigma * stddev;

        for (auto it = pixelstack.begin(); it != pixelstack.end(); ) {
            if (*it<l_limit || *it>u_limit)
                it = pixelstack.erase(it);
            else
                it++;
        }
            
        old_stddev = stddev;
    }
}

void ImageStacking::winsorizedSigmaClip(std::vector<float>& pixelstack, float l_sigma, float u_sigma) {

    int mid_point = pixelstack.size() / 2;

    std::sort(pixelstack.begin(), pixelstack.end());
    float old_stddev = 0;

    for (int iter = 0; iter < 5; iter++) {
        float median = pixelstack[mid_point];
        float stddev = standardDeviation(pixelstack);

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

void ImageStacking::percintileClipping(std::vector<float>& pixelstack, float p_low, float p_high) {

    float med = median(pixelstack);

    if (med == 0)
        return;

    for (auto it = pixelstack.begin(); it != pixelstack.end();) {
        if ((med - *it) / med > p_low || (*it - med) / med > p_high)
            it = pixelstack.erase(it);
        else
            it++;
    }
}

void ImageStacking::pixelRejection(std::vector<float>& pixelstack) {
    using enum Rejection;

    switch (m_rejection) {
    case none:
        return;
    case sigma_clip:
        return sigmaClip(pixelstack, m_sigma_low, m_sigma_high);

    case winsorized_sigma_clip:
        return winsorizedSigmaClip(pixelstack, m_sigma_low, m_sigma_high);

    case percintile_clip:
        return percintileClipping(pixelstack, 0.1f, 0.9f);

    default:
        return;
    }
}

float ImageStacking::pixelIntegration(std::vector<float>& pixelstack) {

    switch (m_integration) {
    case Integration::average:
        return mean(pixelstack);

    case Integration::median:
        return median(pixelstack);

    case Integration::min:
        return min(pixelstack);

    case Integration::max:
        return max(pixelstack);
    default:
        return 0.0f;
    }
}

void ImageStacking::openFiles() {

    m_imgfile_vector.reserve(m_file_paths.size());

    for (int i = 0; i < m_file_paths.size(); ++i) {
        std::unique_ptr<FITS> fits(std::make_unique<FITS>());
        fits->open(m_file_paths[i]);
        m_imgfile_vector.push_back(std::move(fits));
    }
}

bool ImageStacking::isFilesSameDimenisions() {
    int r_rows = 0, r_cols = 0, r_channels = 1;

    FITS fits;
    for (int i = 0; i < m_file_paths.size(); ++i) {
        fits.open(m_file_paths[i]);

        if (i == 0) {
            r_rows = fits.rows();
            r_cols = fits.cols();
            r_channels = fits.channels();
        }

        else {
            if (r_rows != fits.rows())
                return false;
            if (r_cols != fits.cols())
                return false;
            if (r_channels != fits.channels())
                return false;
        }

        fits.close();
    }

    return true;
}

void ImageStacking::closeFiles() {

    for (auto& ifp : m_imgfile_vector)
        ifp->close();
}

void ImageStacking::computeScaleEstimators() {
    using enum Normalization;

    if (m_normalization == none || m_file_paths.size() == 0)
        return m_iss.emitText("Normalization: none");

    m_iss.emitText("Computing Scale Estimators...");
    //m_iss.emitText("Scale Estimator: ");

    std::vector<std::array<float, 3>> mse(m_file_paths.size()); //scale estimators

    m_le.resize(m_file_paths.size());
    m_sf.resize(m_file_paths.size());

    Image32 temp;
    FITS fits;

    for (int i = 0; i < m_file_paths.size(); ++i) {
        fits.open(m_file_paths[i]);
        fits.readAny(temp);
        for (int ch = 0; ch < temp.channels(); ++ch) {
            if (m_normalization == additive_scaling || m_normalization == multiplicative_scaling) {
                m_le[i][ch] = temp.computeMedian(ch, true);
                mse[i][ch] = temp.computeBWMV(ch, m_le[i][ch], true);
                //mle[i][ch] = temp.Median(ch);
                m_sf[i][ch] = mse[0][ch] / mse[i][ch];
            }
            else
                m_le[i][ch] = temp.computeMedian(ch, true);
        }
        fits.close();
    }

    std::array<QString, 5> n = { "none", "additive", "multiplicative", "additive scaling","multiplicative_scaling" };

    m_iss.emitText("Normalization: " + n[int(m_normalization)]);
}

Status ImageStacking::stackImages(const FileVector& paths, Image32& output) {

    if (paths.size() < 2)
        return { false, "Must have at least two frames to stack." };
    
    m_file_paths = paths;

    if (!isFilesSameDimenisions()) {
        m_imgfile_vector.clear();
        return { false, "Frames must be of same dimensions." };
    }
    
    computeScaleEstimators();
    openFiles();

    output = Image32(m_imgfile_vector[0]->rows(), m_imgfile_vector[0]->cols(), m_imgfile_vector[0]->channels());

    PixelRows pixel_rows(m_imgfile_vector.size(), output.cols(), *this);

    std::vector<float> pixelstack(pixel_rows.count());

    int max_threads = (omp_get_max_threads() < 4) ? omp_get_max_threads() : 4;

    m_iss.emitText("Stacking " + QString::number(m_file_paths.size()) + " Images...");

    for (uint32_t ch = 0; ch < output.channels(); ++ch) {

        for (int y = 0; y < output.rows(); ++y) {

            pixel_rows.fill({ 0, y, ch });

#pragma omp parallel for firstprivate(pixelstack) num_threads(max_threads)
            for (int x = 0; x < output.cols(); ++x) {

                pixel_rows.fillPixelStack(pixelstack, x, ch);

                pixelRejection(pixelstack);

                output(x,y,ch) = pixelIntegration(pixelstack);

            }

            m_iss.emitProgress(((ch + 1) * (y + 1) * 100) / (output.channels() * output.rows()));
        }
    }

    if (m_normalization != Normalization::none)
        output.normalize();

    closeFiles();

    m_file_paths.clear();
    m_imgfile_vector.clear();

    return { true, "" };
}







void ImageStackingWeightMap::PixelRows_t::fillPixelStack(Pixelstack_t& pixelstack, int x, int ch) {

    using enum Normalization;

    if (pixelstack.size() != pixelstack.capacity())
        pixelstack.resize(pixelstack.capacity());

    Pixelstack ps(pixelstack.size());

    PixelRows::fillPixelStack(ps, x, ch);

    for (int i = 0; i < ps.size(); ++i) {
        pixelstack[i].value = ps[i];
        pixelstack[i].img_num = i;
    }
}



float ImageStackingWeightMap::mean(const Pixelstack_t& pixelstack) {

    float mean = 0;

    for (const Pixel_t& pixel : pixelstack)
        mean += pixel.value;

    return mean / pixelstack.size();
}

float ImageStackingWeightMap::standardDeviation(const Pixelstack_t& pixelstack) {

    float mean = 0;
    for (const Pixel_t& pixel : pixelstack)
        mean += pixel.value;

    mean /= pixelstack.size();

    double d;
    double var = 0;
    for (const Pixel_t& pixel : pixelstack) {
        d = pixel.value - mean;
        var += d * d;
    }

    return sqrtf(var / pixelstack.size());
}

float ImageStackingWeightMap::median(Pixelstack_t& pixelstack) {
    std::nth_element(pixelstack.begin(), pixelstack.begin() + pixelstack.size() / 2, pixelstack.end(), Pixel_t());

    return pixelstack[pixelstack.size() / 2].value;
}

float ImageStackingWeightMap::min(const Pixelstack_t& pixelstack) {

    float min = std::numeric_limits<float>::max();

    for (Pixel_t pixel : pixelstack)
        if (pixel.value < min)
            min = pixel.value;

    return min;
}

float ImageStackingWeightMap::max(const Pixelstack_t& pixelstack) {

    float max = std::numeric_limits<float>::min();

    for (Pixel_t pixel : pixelstack)
        if (pixel.value > max)
            max = pixel.value;

    return max;
}


void ImageStackingWeightMap::sigmaClip(const ImagePoint& point, Pixelstack_t& pixelstack, float l_sigma, float u_sigma) {

    std::sort(pixelstack.begin(), pixelstack.end(), Pixel_t());
    float old_stddev = 0;

    for (int iter = 0; iter < 5; ++iter) {
        float median = pixelstack[pixelstack.size() / 2].value;
        float stddev = standardDeviation(pixelstack);

        if (stddev == 0 || (old_stddev - stddev) == 0)  break;

        float l_limit = median - l_sigma * stddev;
        float u_limit = median + u_sigma * stddev;

        for (auto it = pixelstack.begin(); it != pixelstack.end();) {
            if (it->value < l_limit || u_limit < it->value)
                it = pixelstack.erase(it);
            else
                it++;
        }

        old_stddev = stddev;
    }

    for (auto pixel : pixelstack)
        m_weight_maps[pixel.img_num](point) = 255;
}

void ImageStackingWeightMap::winsorizedSigmaClip(const ImagePoint& point, Pixelstack_t& pixelstack, float l_sigma, float u_sigma) {

    int mid_point = pixelstack.size() / 2;

    std::sort(pixelstack.begin(), pixelstack.end(), Pixel_t());

    Pixelstack_t copy(pixelstack);

    float old_stddev = 0;

    for (int iter = 0; iter < 5; iter++) {
        float median = pixelstack[mid_point].value;
        float stddev = standardDeviation(pixelstack);

        if (stddev == 0 || (old_stddev - stddev) == 0) break;

        float u_thresh = median + u_sigma * stddev;
        for (int upper = mid_point; upper < pixelstack.size(); ++upper)
            if (pixelstack[upper].value > u_thresh)
                pixelstack[upper].value = pixelstack[upper - 1].value;


        float l_thresh = median - l_sigma * stddev;
        for (int lower = mid_point; lower >= 0; lower--)
            if (pixelstack[lower].value < l_thresh) 
                pixelstack[lower].value = pixelstack[lower + 1].value;

        old_stddev = stddev;
    }

    for (int i = 0; i < copy.size(); ++i)
        m_weight_maps[pixelstack[i].img_num](point) = 255 * (1 - (abs(pixelstack[i].value - copy[i].value) / math::max(pixelstack[i].value, copy[i].value)));
}


void ImageStackingWeightMap::pixelRejection(const ImagePoint& point, Pixelstack_t& pixelstack) {

    using enum Rejection;

    switch (m_rejection) {

    case sigma_clip:
        return sigmaClip(point, pixelstack, m_sigma_low, m_sigma_high);

    case winsorized_sigma_clip:
        return winsorizedSigmaClip(point, pixelstack, m_sigma_low, m_sigma_high);

    //case percintile_clip:
        //return percintileClipping(point, pixelstack, 0.1f, 0.9f);

    default:
        return;
    }
}

float ImageStackingWeightMap::pixelIntegration(Pixelstack_t& pixelstack) {

    switch (m_integration) {
    case Integration::average:
        return mean(pixelstack);

    case Integration::median:
        return median(pixelstack);

    case Integration::min:
        return min(pixelstack);

    case Integration::max:
        return max(pixelstack);
    default:
        return 0.0f;
    }
}

void ImageStackingWeightMap::writeWeightMaps(std::filesystem::path parent_directory) {

    parent_directory /= "WeightMaps";

    if (std::filesystem::exists(parent_directory))
        std::filesystem::remove_all(parent_directory);

    std::filesystem::create_directory(parent_directory);

    for (int i = 0; i < m_weight_maps.size(); ++i) {
        auto path = parent_directory.string() + "//" + m_file_paths[i].stem().string();

        if (path.substr(path.length() - 5) == "_temp")
            path = path.substr(0, path.length() - 5);

        WeightMapImage wmi;
        wmi.create(path);
        wmi.write(m_weight_maps[i]);
    }

    m_weight_maps.clear();
}

//not really correct rejecting too many pixels?!
Status ImageStackingWeightMap::stackImages(const FileVector& paths, Image32& output, std::filesystem::path parent_directory) {

    if (paths.size() < 2)
        return { false, "Must have at least two frames to stack." };

    if (m_rejection == Rejection::none)
        return ImageStacking::stackImages(paths, output);

    m_file_paths = paths;

    if (!isFilesSameDimenisions()) {
        m_imgfile_vector.clear();
        return { false, "Frames must be of same dimensions." };
    }

    computeScaleEstimators();
    openFiles();

    output = Image32(m_imgfile_vector[0]->rows(), m_imgfile_vector[0]->cols(), m_imgfile_vector[0]->channels());

    PixelRows_t pixel_rows(m_imgfile_vector.size(), output.cols(), *this);

    m_weight_maps.resize(m_file_paths.size());
    for (auto& wm : m_weight_maps)
        wm = Image8(output.rows(), output.cols(), output.channels());

    Pixelstack_t pixelstack(m_file_paths.size());

    int max_threads = (omp_get_max_threads() < 4) ? omp_get_max_threads() : 4;

    m_issp->emitText("Stacking " + QString::number(m_file_paths.size()) + " Images & Generate Weight Maps...");

    for (uint32_t ch = 0; ch < output.channels(); ++ch) {

        for (int y = 0; y < output.rows(); ++y) {

            pixel_rows.fill({ 0, y, ch });

#pragma omp parallel for firstprivate(pixelstack) num_threads(max_threads)
            for (int x = 0; x < output.cols(); ++x) {

                pixel_rows.fillPixelStack(pixelstack, x, ch);

                pixelRejection({ x,y,ch }, pixelstack);

                output(x, y, ch) = pixelIntegration(pixelstack);

            }
            m_issp->emitProgress(((ch + 1) * (y + 1) * 100) / (output.channels() * output.rows()));
        }
    }
    
    writeWeightMaps(parent_directory);

    if (m_normalization != Normalization::none)
        output.normalize();

    closeFiles();

    m_file_paths.clear();
    m_imgfile_vector.clear();

    return { true, "" };
}