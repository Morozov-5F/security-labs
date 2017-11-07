#include <iostream>
#include <csetjmp>
#include <cassert>
#include <fstream>

#include <map>

#include <iomanip>
#include <jpeglib.h>

extern "C"
{
#include <jpeglib.h>
}

typedef struct error_mgr_s
{
    struct jpeg_error_mgr errm_jpeg_mgr;
    jmp_buf               errm_setjmp_buf;
} error_mgr_t;

static void
s_error_mgr_exit(j_common_ptr cinfo)
{
    error_mgr_t * error = (error_mgr_t *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);

    longjmp(error->errm_setjmp_buf, 1);
}

static JBLOCKARRAY *
s_read_coefficients(jpeg_decompress_struct * decompress_info, jvirt_barray_ptr * coefficients)
{
    JBLOCKARRAY * row_pointers = nullptr;

    assert(nullptr != coefficients);
    assert(nullptr != decompress_info);

    row_pointers = new JBLOCKARRAY[3];
    assert(nullptr != row_pointers);

    // Iterate through component matrices (Y, Cb, Cr)
    for (JDIMENSION component_num = 0; component_num < decompress_info->num_components; ++component_num)
    {
        jpeg_component_info * component_info = &decompress_info->comp_info[component_num];
        std::cout << "Reading " << component_num << " component. . ." << std::endl;

        size_t height_in_blocks = component_info->height_in_blocks;
        size_t width_in_blocks  = component_info->width_in_blocks;

        std::cout << "Component parameters: " << std::endl;
        std::cout << "          width in blocks: " << component_info->width_in_blocks  << std::endl;
        std::cout << "         height in blocks: " << component_info->height_in_blocks << std::endl;
        std::cout << "              total width: " << component_info->width_in_blocks  * DCTSIZE2 << std::endl;
        std::cout << "             total height: " << component_info->height_in_blocks * DCTSIZE2 << std::endl;
        std::cout << "            v_samp_factor: " << component_info->v_samp_factor << std::endl;

        // Assign component pointer
        row_pointers[component_num] = decompress_info->mem->access_virt_barray((j_common_ptr) &decompress_info,
                                                                              coefficients[component_num],
                                                                              0,
                                                                              component_info->v_samp_factor,
                                                                              FALSE);
        for (JDIMENSION row_num = 0; row_num < height_in_blocks; ++row_num)
        {
            for (JDIMENSION block_num = 0; block_num < width_in_blocks; ++block_num)
            {
                std::cout << std::endl << "** DCT block, component " << component_num << " **" << std::endl;
                for (JDIMENSION i = 0; i < DCTSIZE2; ++i)
                {
                    short dc = row_pointers[component_num][row_num][block_num][i];

                    std::cout << std::setfill(' ') << std::setw(6) << dc;
                    if(i % 8 == 7)
                    {
                        std::cout << std::endl;
                    }
                }
            }
        }
    }
    return row_pointers;
}

static void
s_make_coeff_histogram(jpeg_component_info * component_info, JBLOCKARRAY component)
{
    std::map<short, unsigned> coefficient_map;

    assert(nullptr != component);
    assert(nullptr != component_info);

    std::cout << "Creating JPEG coefficients histogram... ";

    for (auto i = 0; i < component_info->height_in_blocks; ++i)
    {
        for (auto j = 0; j < component_info->width_in_blocks; ++j)
        {
            short prev_byte = component[i][j][0] & 0b00000011;
            for (auto k = 1; k < DCTSIZE2; ++k)
            {
                if (component[i][j][k] == 1 || component[i][j][k] == 0)
                {
                    continue;
                }
                auto coeff = component[i][j][k] & 0b00000011;
                coefficient_map[coeff] += 1;
            }
        }
    }
    std::cout << "Done!" << std::endl;

    std::ofstream out_str("histogram_result.out");
    out_str << "# JPEG coefficients histogram" << std::endl;
    out_str << "# coefficient, frequency" << std::endl;
    for (auto iter = coefficient_map.begin(); iter != coefficient_map.end(); ++iter)
    {
        out_str << iter->first << " " << iter->second << std::endl;
    }
    out_str.close();
    return;
}

int
process_jpeg_file(const char * filename)
{
    JBLOCKARRAY * row_pointers = nullptr;
    struct jpeg_decompress_struct cinfo;
    error_mgr_t                   jerr;

    jvirt_barray_ptr  * jpeg_coefficients = NULL;

    FILE * in_file = nullptr;

    in_file = fopen(filename, "rb");
    if (NULL == in_file)
    {
        std::cout << "Unable to open file " << filename << std::endl;
        return EXIT_FAILURE;
    }

    cinfo.err = jpeg_std_error(&jerr.errm_jpeg_mgr);
    jerr.errm_jpeg_mgr.error_exit = s_error_mgr_exit;

    int jump_ret = setjmp(jerr.errm_setjmp_buf);
    if (0 != jump_ret)
    {
        jpeg_destroy_decompress(&cinfo);
        fclose(in_file);
        std::cout << "Unable to open a JPEG file!" << std::endl;
        return EXIT_FAILURE;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, in_file);

    if (JPEG_HEADER_OK != jpeg_read_header(&cinfo, FALSE))
    {
        std::cout << "Unable to read image header!" << std::endl;
    }

    jpeg_coefficients = jpeg_read_coefficients(&cinfo);
    row_pointers = s_read_coefficients(&cinfo, jpeg_coefficients);

    auto component_index = 0;
    s_make_coeff_histogram(&cinfo.comp_info[component_index], row_pointers[component_index]);

    return EXIT_SUCCESS;
}

int main ()
{
    std::cout << "Hello, World!" << std::endl;
    process_jpeg_file("result_image.jpg");
    return 0;
}