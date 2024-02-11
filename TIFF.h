#pragma once
#include "Image.h"
#include "ImageFile.h"

class TIFF :public ImageFile {

public:
    enum class FieldType : uint16_t {
        BYTE = 1,
        ASCII,
        SHORT,
        LONG,
        RATIONAL,
        SBYTE,
        UNDEFINED,
        SSHORT,
        SLONG,
        FLOAT,
        DOUBLE
    };

    enum class TIFFTAG : uint16_t {
        ImageWidth = 256,
        ImageLength = 257,
        BitsPerSample = 258,
        Compression = 259,
        PhotometricInterpretation = 262,
        StripOffsets = 273,
        SamplesPerPixel = 277,
        RowsPerStrip = 278,
        StripByteCounts = 279,
        XResolution = 282,
        YResolution = 283,
        PlanarConfiguration = 284,
        ResolutionUnit = 296,
        SampleFormat = 339
    };

    enum class PlanarConfig : uint16_t {
        Contiguous = 1,
        Seperate = 2
    };

private:
    struct TIFFHeader {
        char byte_order[2] = { 'I','I' };
        uint16_t identifier = 42;
        uint32_t offset = 0;//location of ifd and directorys in bytes, include size of tiff header in calc
    };

    //size of 12bytes each
    struct DirectoryEntry {
        uint16_t tag;
        uint16_t type;
        uint32_t count;
        uint32_t value_offset;

        bool operator()(DirectoryEntry a, DirectoryEntry b) { return a.tag < b.tag; }
    };

    struct IFD {
        uint16_t num_dir = 0;
        std::vector<DirectoryEntry> directory;
        std::vector<char> offset_data;

        IFD() = default;

        template <typename Image>
        IFD(Image& img, int new_bitdepth, bool planar_contig = true) {
            uint16_t bd_size = abs(new_bitdepth / 8);
            int offset = img.TotalPxCount() * bd_size + 8 + 2 + (12 * 14);

            AddEntry(TIFFTAG::ImageWidth, FieldType::SHORT, 1, img.Cols());
            AddEntry(TIFFTAG::ImageLength, FieldType::SHORT, 1, img.Rows());
            AddEntry(TIFFTAG::Compression, FieldType::SHORT, 1, 1);

            int photo = (img.Channels() == 1) ? 1 : 2;
            AddEntry(TIFFTAG::PhotometricInterpretation, FieldType::SHORT, 1, photo);
            AddEntry(TIFFTAG::RowsPerStrip, FieldType::SHORT, 1, 1);

            int res[2] = { 72,1 };
            AddEntry(TIFFTAG::XResolution, FieldType::RATIONAL, 1, offset, (char*)res, 8);
            offset += 8;
            AddEntry(TIFFTAG::YResolution, FieldType::RATIONAL, 1, offset, (char*)res, 8);
            offset += 8;
            AddEntry(TIFFTAG::ResolutionUnit, FieldType::SHORT, 1, 2);

            uint16_t sample_format = 1;
            if (new_bitdepth == -32)
                sample_format = 3;

            if (img.Channels() == 3) {
                bd_size *= 8;
                uint16_t bda[3] = { bd_size,bd_size,bd_size };
                AddEntry(TIFFTAG::BitsPerSample, FieldType::SHORT, 3, offset, (char*)bda, 6);
                offset += 6;
                uint16_t sf[3] = { sample_format,sample_format,sample_format };
                AddEntry(TIFFTAG::SampleFormat, FieldType::SHORT, 3, offset, (char*)sf, 6);
                offset += 6;
            }

            else {
                AddEntry(TIFFTAG::BitsPerSample, FieldType::SHORT, 1, abs(new_bitdepth));
                AddEntry(TIFFTAG::SampleFormat, FieldType::SHORT, 1, sample_format);
            }

            //rgb cont correct
            if (img.Channels() == 1 || planar_contig) {
                AddEntry(TIFFTAG::StripOffsets, FieldType::LONG, img.Rows(), offset);
                AddEntry(TIFFTAG::StripByteCounts, FieldType::LONG, img.Rows(), offset + img.Rows() * 4);
                AddEntry(TIFFTAG::PlanarConfiguration, FieldType::SHORT, 1, (uint32_t)PlanarConfig::Contiguous);
            }

            //rgb sep correct
            else {
                AddEntry(TIFFTAG::StripOffsets, FieldType::LONG, img.Rows() * img.Channels(), offset);
                AddEntry(TIFFTAG::StripByteCounts, FieldType::LONG, img.Rows() * img.Channels(), offset + (img.Rows() * img.Channels() * 4));
                AddEntry(TIFFTAG::PlanarConfiguration, FieldType::SHORT, 1, (uint32_t)PlanarConfig::Seperate);
            }


            AddEntry(TIFFTAG::SamplesPerPixel, FieldType::SHORT, 1, img.Channels());

        }

        void AddEntry(TIFFTAG tag, FieldType type, uint32_t count, uint32_t value);

        void AddEntry(TIFFTAG tag, FieldType type, uint32_t count, uint32_t offset, char* value, uint32_t num_bytes);

        //r for read, w for write
        void ByteSwapIFD(char mode);

        void Read(std::fstream& stream, bool byteswap);

        void Write(std::fstream& stream);
    };

    IFD ifd;
    int m_data_pos = 8;
    bool byteswap = false;

    PlanarConfig m_planar_config = PlanarConfig::Contiguous;

    std::vector<uint32_t> m_strip_offsets;
    std::vector<uint32_t> m_strip_byte_counts;

public:

    TIFF() = default;

    TIFF(const TIFF& other) {}

    TIFF(TIFF&& other) noexcept : ImageFile(std::move(other)) {

        //move ifd???
        m_data_pos = other.m_data_pos;

        byteswap = other.byteswap;
        m_planar_config = other.m_planar_config;

        m_strip_offsets = std::move(other.m_strip_offsets);
        m_strip_byte_counts = std::move(other.m_strip_byte_counts);

    }

private:
    void GetStripVectors();

    void MakeStripVectors(int bitdepth);

    uint32_t GetTiffParam(TIFFTAG tag);

public:
    uint32_t GetTiffValue(TIFFTAG tag);

    FieldType GetTiffType(TIFFTAG tag);

    uint32_t GetTiffCount(TIFFTAG tag);


    void Open(std::filesystem::path path) override;

    void Create(std::filesystem::path path) override;

    void Close() override;


    template< typename T>
    void ReadScanLine(T* buffer, uint32_t row, uint32_t channel = 0) {

        m_stream.seekg(m_strip_offsets[row] + channel * m_px_count * sizeof(T));
        m_stream.read((char*)buffer, m_strip_byte_counts[row]);
    }

    template<typename T>
    void Read(Image<T>& dst);


    template< typename T>
    void WriteScanLine(T* buffer, uint32_t row, uint32_t channel = 0) {

        m_stream.seekp(m_strip_offsets[row] + channel * m_px_count * sizeof(T));
        m_stream.write((char*)buffer, m_strip_byte_counts[row]);
    }

private:
    template <typename D, typename T>
    void WritePixels_Seperate(const Image<T>& src) {

        std::vector<D> buffer(m_cols);
        for (int ch = 0; ch < m_channels; ++ch) {
            for (int y = 0; y < m_rows; ++y) {
                for (int x = 0; x < m_cols; ++x) {
                    buffer[x] = Pixel<D>::toType(src(x, y, ch));
                }
                WriteScanLine(buffer.data(), y);
            }
        }
    }

    template <typename D, typename T>
    void WritePixels_Contiguous(const Image<T>& src) {

        std::vector<D> buffer(m_cols * m_channels);
        for (int y = 0; y < m_rows; ++y) {
            for (int x = 0, i = 0; x < m_cols; ++x) {

                buffer[i++] = Pixel<D>::toType(src(x, y, 0));
                buffer[i++] = Pixel<D>::toType(src(x, y, 1));
                buffer[i++] = Pixel<D>::toType(src(x, y, 2));

            }
            WriteScanLine(buffer.data(), y);
        }
    }

public:
    template<typename T>
    void Write(const Image<T>& src, int new_bitdepth, bool planar_contiguous = true);
};


