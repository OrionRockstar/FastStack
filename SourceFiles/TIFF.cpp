#include "pch.h"
#include "TIFF.h"

void TIFF::IFD::AddEntry(TIFFTAG tag, FieldType type, uint32_t count, uint32_t value) {
    DirectoryEntry entry;
    entry.tag = (uint16_t)tag;
    entry.type = (uint16_t)type;
    entry.count = count;
    entry.value_offset = value;
    directory.push_back(entry);
    num_dir = directory.size();

    std::sort(directory.begin(), directory.end(), DirectoryEntry());
}

void TIFF::IFD::AddEntry(TIFFTAG tag, FieldType type, uint32_t count, uint32_t offset, char* value, uint32_t num_bytes) {
    DirectoryEntry entry;
    entry.tag = (uint16_t)tag;
    entry.type = (uint16_t)type;
    entry.count = count;
    entry.value_offset = offset;
    directory.push_back(entry);
    num_dir = directory.size();

    std::sort(directory.begin(), directory.end(), DirectoryEntry());

    for (int i = 0; i < num_bytes; ++i)
        offset_data.push_back(value[i]);
}

void TIFF::IFD::ByteSwapIFD(char mode) {

    num_dir = _byteswap_ushort(num_dir);

    for (auto& entry : directory) {
        entry.tag = _byteswap_ushort(entry.tag);

        if (mode == 'r') {
            entry.type = _byteswap_ushort(entry.type);
            entry.count = _byteswap_ulong(entry.count);
        }

        if (entry.type == 3 && entry.count == 1)
            entry.value_offset = _byteswap_ushort(entry.value_offset);

        else if (entry.type == 3 && entry.count > 1)
            entry.value_offset = _byteswap_ulong(entry.value_offset);

        else if (entry.type == 4 || entry.type == 5)
            entry.value_offset = _byteswap_ulong(entry.value_offset);

        if (mode == 'w') {
            entry.type = _byteswap_ushort(entry.type);
            entry.count = _byteswap_ulong(entry.count);
        }
    }

}

void TIFF::IFD::Read(std::fstream& stream, bool byteswap) {

    stream.read((char*)&num_dir, 2);
    if (byteswap)
        num_dir = _byteswap_ushort(num_dir);

    directory.resize(num_dir);
    stream.read((char*)directory.data(), num_dir * sizeof(DirectoryEntry));

    if (byteswap)
        ByteSwapIFD('r');
}

void TIFF::IFD::Write(std::fstream& stream) {

    stream.write((char*)&num_dir, 2);
    stream.write((char*)directory.data(), directory.size() * 12);

    stream.write((char*)offset_data.data(), offset_data.size());
}



void TIFF::getStripVectors() {

    //strip offsets
    int offset = tiffValueOffset(TIFFTAG::StripOffsets);
    m_stream.seekg(offset);
    m_strip_offsets.resize(tiffCount(TIFFTAG::StripOffsets));

    m_stream.read((char*)m_strip_offsets.data(), m_strip_offsets.size() * 4);


    //strip byte counts
    offset = tiffValueOffset(TIFFTAG::StripByteCounts);
    m_stream.seekg(offset);

    m_strip_byte_counts.resize(tiffCount(TIFFTAG::StripByteCounts));

    if (tiffType(TIFFTAG::StripByteCounts) == FieldType::SHORT) {
        std::vector<uint16_t> buffer(m_strip_byte_counts.size());
        m_stream.read((char*)buffer.data(), buffer.size() * 2);
        for (int i = 0; i < m_strip_byte_counts.size(); ++i)
            m_strip_byte_counts[i] = buffer[i];
        return;
    }

    m_stream.read((char*)m_strip_byte_counts.data(), m_strip_byte_counts.size() * 4);

    if (byteswap) {
        for (int i = 0; i < m_strip_offsets.size(); ++i) {
            m_strip_offsets[i] = _byteswap_ulong(m_strip_offsets[i]);
            m_strip_byte_counts[i] = _byteswap_ulong(m_strip_byte_counts[i]);
        }
    }
}

void TIFF::makeStripVectors(ImageType type) {
    int mem_row_size = m_cols * typeSize(type);

    if (m_planar_config == PlanarConfig::Seperate) {

        m_strip_offsets.resize(m_rows * m_channels);
        m_strip_byte_counts.resize(m_rows * m_channels, mem_row_size);

    }

    else if (m_planar_config == PlanarConfig::Contiguous) {

        mem_row_size *= m_channels;
        m_strip_offsets.resize(m_rows);

        m_strip_byte_counts.resize(m_rows, mem_row_size);

        m_strip_offsets[0] = 8;

    }

    for (int i = 1; i < m_strip_offsets.size(); ++i)
        m_strip_offsets[i] = mem_row_size + m_strip_offsets[i - 1];
}


ImageType TIFF::imageTypefromFile() {

    SampleFormat format = SampleFormat(tiffValue(TIFFTAG::SampleFormat));
    switch (tiffValue(TIFFTAG::BitsPerSample)) {
    case 8:
        if (format == SampleFormat::Unsigned)
            return ImageType::UBYTE;
    case 16:
        if (format == SampleFormat::Unsigned)
            return ImageType::USHORT;
    case 32:
        if (format == SampleFormat::Float)
            return ImageType::FLOAT;
    default:
        return ImageType::UBYTE;
    }
}

TIFF::FieldType TIFF::tiffType(TIFFTAG tag) {
    for (auto entry : ifd.directory) {
        if ((int)tag == entry.tag)
            return FieldType(entry.type);
    }
    return FieldType(1);
}

uint32_t TIFF::tiffCount(TIFFTAG tag) {
    for (auto entry : ifd.directory) {
        if ((int)tag == entry.tag)
            return entry.count;
    }
    return 1;
}

uint32_t TIFF::tiffValueOffset(TIFFTAG tag) {
    for (auto entry : ifd.directory) {
        if ((int)tag == entry.tag)
            return entry.value_offset;
    }
    return 1;
}

uint32_t TIFF::tiffValue(TIFFTAG tag) {

    FieldType type = tiffType(tag);
    uint32_t count = tiffCount(tag);
    uint32_t val = tiffValueOffset(tag);

    if (count == 1 && type != FieldType::RATIONAL)
        return val;

    m_stream.seekg(val);

    if (type == FieldType::SHORT) {
        uint16_t sval;
        m_stream.read((char*)&sval, 2);
        return (byteswap) ? _byteswap_ushort(sval) : sval;
    }

    else if (type == FieldType::LONG) {
        m_stream.read((char*)&val, 4);
        return (byteswap) ? _byteswap_ulong(val) : val;;
    }

    else if (type == FieldType::RATIONAL) {
        int numbers[2];
        m_stream.read((char*)&numbers, 8);
        return (byteswap) ? _byteswap_ulong(numbers[0] / numbers[1]) : numbers[0] / numbers[1];
    }

    return 1;
}


void TIFF::open(std::filesystem::path path) {

    ImageFile::open(path);

    TIFFHeader header;
    m_stream.read((char*)&header, sizeof(TIFFHeader));

    if (header.byte_order[0] == 'M' && header.byte_order[1] == 'M') {
        byteswap = true;
        header.identifier = _byteswap_ushort(header.identifier);
        header.offset = _byteswap_ulong(header.offset);
    }

    if (header.identifier != 42)
        return;

    m_stream.seekg(header.offset);
    ifd.Read(m_stream, byteswap);

    m_rows = tiffValue(TIFFTAG::ImageLength);
    m_cols = tiffValue(TIFFTAG::ImageWidth);
    m_channels = tiffValue(TIFFTAG::SamplesPerPixel);

    m_img_type = imageTypefromFile();
    //m_bitdepth = tiffValue(TIFFTAG::BitsPerSample);

    m_planar_config = PlanarConfig(tiffValue(TIFFTAG::PlanarConfiguration));

    m_px_count = m_rows * m_cols;

    getStripVectors();

    //SetBuffer();
    resizeBuffer(m_strip_byte_counts[0]);
}

void TIFF::create(std::filesystem::path path) {

    path += ".tiff";

    ImageFile::create(path);
}

void TIFF::close() {

    ImageFile::close();

    byteswap = false;
    ifd = IFD();
    m_data_pos = 8;

    m_strip_offsets.clear();
    m_strip_byte_counts.clear();
}


template<typename T>
void TIFF::read(Image<T>& dst) {

    m_stream.seekg(m_data_pos);

    dst = Image<T>(rows(), cols(), channels());

    if (channels() == 1 || m_planar_config == PlanarConfig::Seperate) {

        for (int ch = 0; ch < dst.channels(); ++ch) {
            for (int row = 0; row < rows(); ++row) {
                readScanLine(&dst(0, row, ch), row, ch);
            }
        }
    }

    else if (channels() == 3 && m_planar_config == PlanarConfig::Contiguous) {
        std::vector<T> buffer(cols() * channels());
        for (int row = 0; row < rows(); ++row) {
            readScanLine(buffer.data(), row);
            for (int c = 0, i = 0; c < m_cols; ++c) {
                dst(c, row, 0) = buffer[i++];
                dst(c, row, 1) = buffer[i++];
                dst(c, row, 2) = buffer[i++];
            }
        }
    }

    if (byteswap)
        for (auto& p : dst)
            p = _byteswap_ushort(p);

    if (dst.type() == ImageType::FLOAT)
        dst.normalize();

    close();
}
template void TIFF::read(Image8&);
template void TIFF::read(Image16&);
template void TIFF::read(Image32&);

void TIFF::readAny(Image32& dst) {

    m_stream.seekg(m_data_pos);

    dst = Image32(m_rows, m_cols, m_channels);

    switch (imageType()) {
    case ImageType::UBYTE: {
        Image8 temp;
        read(temp);
        for (int el = 0; el < dst.totalPxCount(); ++el)
            dst[el] = Pixel<float>::toType(temp[el]);
        break;
    }

    case ImageType::USHORT: {
        Image16 temp;
        read(temp);
        for (int el = 0; el < dst.totalPxCount(); ++el)
            dst[el] = Pixel<float>::toType(temp[el]);
        break;
    }

    case ImageType::FLOAT: {
        read(dst);
        dst.normalize();
        break;
    }
    }
    close();
}

template<typename T>
void TIFF::write(const Image<T>& src, ImageType new_type, bool planar_contiguous) {

    using enum PlanarConfig;

    TIFFHeader header;
    header.offset = src.totalPxCount() * abs(typeSize(new_type)) + 8;

    m_stream.write((char*)&header, sizeof(header));

    m_rows = src.rows();
    m_cols = src.cols();
    m_channels = src.channels();

    m_planar_config = (planar_contiguous) ? Contiguous : Seperate;

    makeStripVectors(new_type);
    resizeBuffer(m_strip_byte_counts[0]);

    switch (new_type) {
    case ImageType::UBYTE: {
            if (m_channels == 1 || m_planar_config == Seperate)
                writePixels_Seperate<uint8_t>(src);
            else if (m_planar_config == Contiguous)
                writePixels_Contiguous<uint8_t>(src);

            break;
        }

    case ImageType::USHORT: {
            if (m_channels == 1 || m_planar_config == Seperate)
                writePixels_Seperate<uint16_t>(src);
            else if (m_planar_config == Contiguous)
                writePixels_Contiguous<uint16_t>(src);

            break;
        }

    case ImageType::FLOAT: {
            if (m_channels == 1 || m_planar_config == Seperate)
                writePixels_Seperate<float>(src);
            else if (m_planar_config == Contiguous)
                writePixels_Contiguous<float>(src);

            break;
        }
    }

    ifd = IFD(src, new_type, planar_contiguous);

    ifd.Write(m_stream);

    m_stream.write((char*)m_strip_offsets.data(), 4 * m_strip_offsets.size());
    m_stream.write((char*)m_strip_byte_counts.data(), 4 * m_strip_byte_counts.size());

    close();
}
template void TIFF::write(const Image8&, ImageType, bool);
template void TIFF::write(const Image16&, ImageType, bool);
template void TIFF::write(const Image32&, ImageType, bool);