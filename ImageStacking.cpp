#include "pch.h"
#include "ImageStacking.h"

//need to scale both drizzle and not drizzle
//drizzle need aligned stats but unaligned image

ImageStacking::PixelRows::PixelRows(int num_imgs, int cols, ImageStacking& isp) : m_cols(cols), m_num_imgs(num_imgs), m_isp(&isp) {
    m_size = num_imgs * m_cols;
    m_pixels = std::vector<float>(m_size);
}

void ImageStacking::PixelRows::Fill(const Point<>& start_point) {

    //#pragma omp parallel for num_threads(m_max_threads)
                //for (int i = 0; i < m_isp->m_fits_vector.size(); ++i) {
                    //m_isp->m_fits_vector[i].ReadSome_Any(&(*this)(0, i), start_point, m_cols);
                //}
#pragma omp parallel for num_threads(m_max_threads)
    for (int i = 0; i < m_isp->m_imgfile_vector.size(); ++i) {
        switch (m_isp->m_imgfile_vector[i]->Type()) {
        case FileType::FITS:
            reinterpret_cast<FITS*>(m_isp->m_imgfile_vector[i].get())->ReadSome_Any(&(*this)(0, i), start_point, m_cols);
            break;
        }
    }

}

void ImageStacking::PixelRows::FillPixelStack(std::vector<float>& pixelstack, int x, int ch) {

    using enum Normalization;

    if (pixelstack.size() != pixelstack.capacity())
        pixelstack.resize(pixelstack.capacity());

    switch (m_isp->m_normalization) {
    case additive:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = Pixel<float>::toType((*this)(x, i)) - m_isp->mle[i][ch] + m_isp->mle[0][ch];

        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = (*this)(x, i) - m_isp->mle[i][ch] + m_isp->mle[0][ch];
        return;

    case multiplicative:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = (*this)(x, i) * (m_isp->mle[0][ch] / m_isp->mle[i][ch]);
        return;

    case additive_scaling:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = m_isp->msf[i][ch] * ((*this)(x, i) - m_isp->mle[i][ch]) + m_isp->mle[0][ch];
        return;

    case multiplicative_scaling:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = m_isp->msf[i][ch] * (*this)(x, i) * (m_isp->mle[0][ch] / m_isp->mle[i][ch]);
        return;

    case none:
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = (*this)(x, i);
        return;
    }

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

float Min(const std::vector<float>& pixelstack) {

    float min = std::numeric_limits<float>::max();

    for (const float& pixel : pixelstack)
        if (pixel < min)
            min = pixel;

    return min;
}

float Max(const std::vector<float>& pixelstack) {

    float max = std::numeric_limits<float>::min();

    for (const float& pixel : pixelstack)
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

void ImageStacking::PercintileClipping(std::vector<float>& pixelstack, float p_low, float p_high) {

    float median = Median(pixelstack);

    if (median == 0)
        return;

    for (auto it = pixelstack.begin(); it != pixelstack.end(); ++it)
        if ((median - *it) / median > p_low || (*it - median) / median > p_high) {
            pixelstack.erase(it);
            it--;
        }
}

void ImageStacking::SigmaClip(std::vector<float>& pixelstack, float l_sigma, float u_sigma) {

    std::sort(pixelstack.begin(), pixelstack.end());
    float old_stddev = 0;

    for (int iter = 0; iter < 5; iter++) {
        float median = pixelstack[pixelstack.size() / 2];
        float stddev = StandardDeviation(pixelstack);

        if (stddev == 0 || (old_stddev - stddev) == 0)  break;

        float l_limit = median - l_sigma * stddev;
        float u_limit = median + u_sigma * stddev;
        //RemoveOutliers(pixelstack, median - l_sigma * stddev, median + u_sigma * stddev);
        for (auto it = pixelstack.begin(); it != pixelstack.end(); ++it)
            if (*it<l_limit || *it>u_limit) {
                pixelstack.erase(it);
                it--;
            }

        old_stddev = stddev;
    }
}

//weight map needs to include channels
//find alternative to individual weight maps/ use weight maps only for drizzle
// have vector of points
//rejection vec  rej[i] =(x,y,ch), i being nth image in stavk,
// rej[i] = (image#,x,y,ch)
//store coordinates in meta text
//below wrong, see below
//store rejected pixels in seperate vec in sigma clip

void SetWeightMaps(std::vector<Image8>& weight_maps, int x, int y, int ch, const std::vector<float>& orig_pix, const std::vector<float>& mod_pix) {
    //if (mod_pix.size() == orig_pix.size())
        //return;

    for (int i = 0; i < orig_pix.size(); ++i) {
        for (const float& m : mod_pix) {
            if (orig_pix[i] == m)
                goto not_outlier;
        }
        weight_maps[i](x, y, ch) = 0;

    not_outlier:;
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

void ImageStacking::PixelRejection(std::vector<float>& pixelstack) {
    using enum Rejection;

    switch (m_rejection) {
    case none:
        return;
    case sigma_clip:
        return SigmaClip(pixelstack, m_l_sigma, m_u_sigma);

    case winsorized_sigma_clip:
        return WinsorizedSigmaClip(pixelstack, m_l_sigma, m_u_sigma);

    case percintile_clip:
        return PercintileClipping(pixelstack, 0.1, 0.9);

    default:
        return;
    }
}

float ImageStacking::PixelIntegration(std::vector<float>& pixelstack) {

    using enum Integration;

    switch (m_integration) {
    case average:
        return Mean(pixelstack);

    case median:
        return Median(pixelstack);

    default:
        return 0.0f;
    }
}

void ImageStacking::OpenFiles() {

    m_imgfile_vector.reserve(m_file_paths.size());

    for (int i = 0; i < m_file_paths.size(); ++i) {
        std::unique_ptr<FITS> fits(std::make_unique<FITS>());
        //FITS* fits = new FITS;
        fits->Open(m_file_paths[i]);
        m_imgfile_vector.push_back(std::move(fits));
    }
}

bool ImageStacking::isFilesSameDimenisions() {
    int r_rows = 0, r_cols = 0, r_channels = 1;


    for (auto imgf = m_imgfile_vector.begin(); imgf != m_imgfile_vector.end(); ++imgf) {
        if (imgf == m_imgfile_vector.begin()) {
            r_rows = (*imgf)->Rows();
            r_cols = (*imgf)->Cols();
            r_channels = (*imgf)->Channels();
        }

        else {
            if (r_rows != (*imgf)->Rows())
                return false;
            if (r_cols != (*imgf)->Cols())
                return false;
            if (r_channels != (*imgf)->Channels())
                return false;
        }
    }

    return true;
}

void ImageStacking::CloseFiles() {

    for (auto& ifp : m_imgfile_vector)
        ifp->Close();

}


void ImageStacking::ComputeScaleEstimators() {
    using enum Normalization;

    if (m_normalization == none || m_file_paths.size() == 0)
        return;

    std::vector<std::array<float, 3>> mse(m_file_paths.size()); //scale estimators

    mle.resize(m_file_paths.size());
    msf.resize(m_file_paths.size());

    Image32 temp;
    FITS fits;

    long fp[3] = { 1,1,1 };

    for (int i = 0; i < m_file_paths.size(); ++i) {
        fits.Open(m_file_paths[i]);
        fits.ReadAny(temp);
        for (int ch = 0; ch < temp.Channels(); ++ch) {
            if (m_normalization == additive_scaling || m_normalization == multiplicative_scaling) {
                mle[i][ch] = temp.ComputeMedian(ch, true);
                mse[i][ch] = temp.ComputeBWMV(ch, mle[i][ch], true);
                //mle[i][ch] = temp.Median(ch);
                msf[i][ch] = mse[0][ch] / mse[i][ch];
            }
            else
                mle[i][ch] = temp.ComputeMedian(ch, true);
        }
        fits.Close();
    }
}

void ImageStacking::GenerateWeightMaps_forDrizzle(FileVector paths) {
    m_file_paths = paths;

    using enum Rejection;

    if (m_normalization != Normalization::none)
        ComputeScaleEstimators();

    OpenFiles();

    int rows = m_imgfile_vector[0]->Rows(), cols = m_imgfile_vector[0]->Cols(), channels = m_imgfile_vector[0]->Channels();

    PixelRows pixel_rows(m_imgfile_vector.size(), cols, *this);

    std::vector<float> pixelstack(m_imgfile_vector.size());
    std::vector<float> original_pix(pixelstack.size());

    Image8Vector weight_maps(pixelstack.size());

    for (auto& wm : weight_maps) {
        wm = Image8(rows, cols);
        wm.FillValue(0);
    }

    int max_threads = (omp_get_max_threads() < 4) ? omp_get_max_threads() : 4;

    for (int ch = 0; ch < channels; ++ch) {

        std::array<int, 3> spixel = { 0,0,ch };

        for (int y = 0; y < rows; ++y, ++spixel[1]) {

            //pixel_rows.Fill(spixel);

#pragma omp parallel for firstprivate(pixelstack, original_pix) num_threads(max_threads)
            for (int x = 0; x < cols; ++x) {

                pixel_rows.FillPixelStack(pixelstack, x, ch);
                memcpy(original_pix.data(), pixelstack.data(), pixelstack.size() * 4);

                switch (m_rejection) {
                case none:
                    break;
                case sigma_clip:
                    SigmaClip(pixelstack, m_l_sigma, m_u_sigma);
                    break;
                case winsorized_sigma_clip:
                    WinsorizedSigmaClip(pixelstack, m_l_sigma, m_u_sigma);
                    break;
                case percintile_clip:
                    PercintileClipping(pixelstack, 0.1, 0.9);
                    break;
                default:
                    break;
                }

                SetWeightMaps(weight_maps, x, y, ch, original_pix, pixelstack);

            }
        }
    }

    for (int i = 0; i < paths.size(); ++i) {
        //Bitmap bmp;
        std::cout << paths[i].parent_path().append(paths[i].stem().string()).concat(".bmp") << "\n";
        //bmp.Create(paths[i].parent_path().append(paths[i].stem().string()).string() + ".bmp");

        //bmp.Write(weight_maps[i], true);
    }

    //for (auto& fits : m_fits_vector)
        //fits.Close();
}

Status ImageStacking::IntegrateImages(FileVector paths, Image32& output) {

    if (paths.size() < 2)
        return { false, "Must have at least two frames to stack." };
    
    m_file_paths = paths;

    ComputeScaleEstimators();

    OpenFiles();

    if (!isFilesSameDimenisions()) {
        m_imgfile_vector.clear();
        return { false, "Frames must be of same dimensions." };
    }

    output = Image32(m_imgfile_vector[0]->Rows(), m_imgfile_vector[0]->Cols(), m_imgfile_vector[0]->Channels());

    PixelRows pixel_rows(m_imgfile_vector.size(), output.Cols(), *this);

    std::vector<float> pixelstack(pixel_rows.NumberofImages());

    int max_threads = (omp_get_max_threads() < 4) ? omp_get_max_threads() : 4;

    for (int ch = 0; ch < output.Channels(); ++ch) {

        for (int y = 0; y < output.Rows(); ++y) {

            pixel_rows.Fill(Point<>(0, y, ch));

#pragma omp parallel for firstprivate(pixelstack) num_threads(max_threads)
            for (int x = 0; x < output.Cols(); ++x) {

                pixel_rows.FillPixelStack(pixelstack, x, ch);

                PixelRejection(pixelstack);

                output(x,y,ch) = PixelIntegration(pixelstack);

            }
        }
    }

    if (m_normalization != Normalization::none)
        output.Normalize();

    CloseFiles();

    m_file_paths.clear();
    m_imgfile_vector.clear();

    return { true, "" };
}
