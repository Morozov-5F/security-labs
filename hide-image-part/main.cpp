#include <iostream>
#include <SFML/Graphics.hpp>

#include <zlib.h>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "feistel/FeistelNet.h"
}

#include <cassert>

#define GET_BIT(byte, index)        (((byte) >> (index)) & 1)
#define SET_BIT(byte, index, value) (byte ^= (-(value) ^ (byte)) & (1 << (index)));

static int
s_compress_buffer(sf::Uint8 * buffer, size_t buffer_size, sf::Uint8 ** result, size_t * result_size)
{
    int ret = EXIT_SUCCESS;
    z_stream deflate_stream;

    sf::Uint8 * compressed_buffer      = nullptr;
    std::size_t compressed_buffer_size = 0;

    assert(nullptr != buffer);
    assert(nullptr != result);
    assert(nullptr != result_size);

    // Assign input values
    deflate_stream.next_in  = buffer;
    deflate_stream.avail_in = buffer_size;

    // Initialize internal
    deflate_stream.data_type = Z_BINARY;
    deflate_stream.zalloc    = Z_NULL;
    deflate_stream.zfree     = Z_NULL;
    deflate_stream.opaque    = Z_NULL;

    if (Z_OK != deflateInit(&deflate_stream, Z_BEST_COMPRESSION))
    {
        std::cout << "zLib error: " << deflate_stream.msg << std::endl;
        return EXIT_FAILURE;
    }

    compressed_buffer_size = deflateBound(&deflate_stream, buffer_size);
    compressed_buffer = new uint8_t[compressed_buffer_size];
    assert(nullptr != compressed_buffer);

    // Assign output buffer values
    deflate_stream.avail_out = compressed_buffer_size;
    deflate_stream.next_out  = compressed_buffer;

    if (Z_STREAM_END != deflate(&deflate_stream, Z_FINISH))
    {
        std::cout << "zLib deflate error: " << deflate_stream.msg << std::endl;
        delete compressed_buffer;
        return EXIT_FAILURE;
    }

    deflateEnd(&deflate_stream);

    *result      = compressed_buffer;
    *result_size = deflate_stream.total_out;

    return EXIT_SUCCESS;
}

static int
s_decompress_buffer(sf::Uint8 * buffer, size_t buffer_size, sf::Uint8 ** result, size_t max_result_size,
                    size_t * result_size)
{
    int ret = EXIT_SUCCESS;
    z_stream inflate_stream;

    sf::Uint8 * decompressed_buffer      = nullptr;
    std::size_t decompressed_buffer_size = 0;

    assert(nullptr != buffer);
    assert(nullptr != result);
    assert(nullptr != result_size);

    // Assign input values
    inflate_stream.next_in  = buffer;
    inflate_stream.avail_in = buffer_size;

    decompressed_buffer_size = max_result_size;
    decompressed_buffer      = new uint8_t[decompressed_buffer_size];
    assert(nullptr != decompressed_buffer);

    // Assign output buffer values
    inflate_stream.avail_out = decompressed_buffer_size;
    inflate_stream.next_out  = decompressed_buffer;

    // Initialize internal
    inflate_stream.data_type = Z_BINARY;
    inflate_stream.zalloc    = Z_NULL;
    inflate_stream.zfree     = Z_NULL;
    inflate_stream.opaque    = Z_NULL;
    inflate_stream.msg       = Z_NULL;

    if (Z_OK != inflateInit(&inflate_stream))
    {
        std::cout << "zLib error: " << inflate_stream.msg << std::endl;
        return EXIT_FAILURE;
    }

    if (Z_STREAM_END != inflate(&inflate_stream, Z_NO_FLUSH))
    {
        std::cout << "zLib inflate error: " << inflate_stream.msg << std::endl;
        delete decompressed_buffer;
        return EXIT_FAILURE;
    }

    inflateEnd(&inflate_stream);

    *result      = decompressed_buffer;
    *result_size = inflate_stream.total_out;

    return EXIT_SUCCESS;
}

int main(int argc, char const * argv[])
{
    std::string source_path = "source_image.png";

    sf::Image source_image;
    if (!source_image.loadFromFile(source_path))
    {
        std::cout << "Unable to load image '" << source_path.c_str() << "'" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Vector2u source_dimensions = source_image.getSize();
    size_t image_size = source_dimensions.x * source_dimensions.y;

    sf::Rect<int> area_to_hide(148, 178, 778, 246);

    // Upper bound -- embedding area of size 194928 into image (33.0485% from source image)
    int area_size = area_to_hide.width * area_to_hide.height;
    auto area_pixels = new sf::Uint32[area_size];

    auto image_pixels = new sf::Uint32[image_size];
    const sf::Uint8 * image_bytes = source_image.getPixelsPtr();
    memcpy((sf::Uint8 *)image_pixels, image_bytes, source_dimensions.x * source_dimensions.y);

    for (size_t i = 0, j = 0; i < image_size; ++i)
    {
        size_t x = i % source_dimensions.x, y = i / source_dimensions.x;
        if (area_to_hide.contains(x, y))
        {
            area_pixels[j++] = image_pixels[i];
            image_pixels[i]  = 0xFF000000;
        }
    }

    sf::Uint8 * compressed_area = nullptr;
    size_t      compressed_area_size = 0;
    if (EXIT_SUCCESS != s_compress_buffer((sf::Uint8 *)area_pixels, area_size * 4u, &compressed_area, &compressed_area_size))
    {
        std::cout << "Unable to compress image area!" << std::endl;
        return EXIT_FAILURE;
    }

    float compression_ratio = 100.f - (compressed_area_size / 4.f) / area_size * 100.f;
    std::cout << "Total pixels:          " << image_size << " pixels" << std::endl;
    std::cout << "Area size:             " << area_size << " pixels" << std::endl;
    std::cout << "Compressed area size: ~" << compressed_area_size / 4 << " pixels (" << compression_ratio << "% compressed)" << std::endl << std::endl;

    float image_part = (float)area_size / image_size * 100.f;
    std::cout << "Embedding area of size " << area_size << " into image (" << image_part << "% from source image)" << std::endl;
    bool success = false;
    for (size_t i = 0, k = 0; i < image_size; ++i)
    {
        size_t x = i % source_dimensions.x, y = i / source_dimensions.x;
        if (area_to_hide.contains(x, y))
        {
            continue;
        }

        for (size_t j = 0; j < 4; ++j)
        {
            SET_BIT(image_pixels[i], j * 8,     GET_BIT(compressed_area[k], j * 2));
            SET_BIT(image_pixels[i], j * 8 + 1, GET_BIT(compressed_area[k], j * 2 + 1));
        }

        if (++k >= compressed_area_size)
        {
            success = true;
            break;
        }
    }
    if (!success)
    {
        std::cout << "Unable to embed area into image" << std::endl;
        return  EXIT_FAILURE;
    }
    auto extracted_compressed = new sf::Uint8[compressed_area_size];
    for (size_t i = 0, k = 0; i < image_size; ++i)
    {
        size_t x = i % source_dimensions.x, y = i / source_dimensions.x;

        if (area_to_hide.contains(x, y))
        {
            continue;
        }

        for (size_t j = 0; j < 4; ++j)
        {
            SET_BIT(extracted_compressed[k], j * 2, GET_BIT(image_pixels[i], j * 8));
            SET_BIT(extracted_compressed[k], j * 2 + 1, GET_BIT(image_pixels[i], j * 8 + 1));
        }

        if (++k >= compressed_area_size)
        {
            break;
        }
    }

    sf::Texture output_image;
    output_image.create(source_dimensions.x, source_dimensions.y);
    output_image.update((sf::Uint8 *)image_pixels);
    auto result_image = output_image.copyToImage();
    result_image.saveToFile("intermediate_image.png");

    sf::Uint8 * extracted_area = nullptr;
    size_t extracted_area_size = 0;
    if (EXIT_SUCCESS != s_decompress_buffer(extracted_compressed, compressed_area_size, &extracted_area, area_size * 4u,
                                            &extracted_area_size))
    {
        std::cout << "Unable to decompress extracted area" << std::endl;
        return EXIT_FAILURE;
    }

    if (extracted_area_size != area_size * 4u)
    {
        std::cout << "Embedded image is not correct!" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Embedded part verified" << std::endl;

    auto extracted_area_pixels = (sf::Uint32 *)extracted_area;
    for (size_t i = 0; i < area_size; ++i)
    {
        size_t x = i % area_to_hide.width + area_to_hide.left, y = i / area_to_hide.width + area_to_hide.top;

        size_t j = y * source_dimensions.x + x;
        image_pixels[j] = extracted_area_pixels[i];
    }

    output_image.update((sf::Uint8 *)image_pixels);
    result_image = output_image.copyToImage();
    result_image.saveToFile("result_image.png");

    delete [] image_pixels;
    delete [] area_pixels;
    return EXIT_SUCCESS;
}