#include "pch.h"
#include "jImageFileLoader.h"
#include "lodepng.h"

extern "C"
{
#include "jpeglib2/jpeglib.h"
#include <setjmp.h>
}

jImageFileLoader* jImageFileLoader::_instance = nullptr;

jImageFileLoader::jImageFileLoader()
{
}


jImageFileLoader::~jImageFileLoader()
{
}

typedef struct
{
	struct jpeg_error_mgr Pub;	// public fields 
	jmp_buf JumpBuffer;			// for return to caller
} LogErrorManager;

void LogMessage(j_common_ptr cinfo)
{
	LogErrorManager* err = (LogErrorManager*)cinfo->err;

	// If we got 53, this is not a jpg file, so don't gen a msg, since we want to try other formats
	if (err->Pub.msg_code != 53)
	{
		char buffer[JMSG_LENGTH_MAX];

		// Create the message
		(*cinfo->err->format_message) (cinfo, buffer);

		// Convert to Unicode
		int Len = (int)strlen(buffer) + 1;
		WCHAR* unicodeBuffer = new WCHAR[Len];

		MultiByteToWideChar(CP_ACP, WC_DEFAULTCHAR, buffer, Len, unicodeBuffer, Len);
		//CoreLog::Information(std::wstring(unicodeBuffer));
		delete unicodeBuffer;
	}

	// Return control to the setjmp point 
	longjmp(err->JumpBuffer, 1);
}

// Ignore warning messages
void LogMessageDiscard(j_common_ptr cinfo)
{

}


int read_JPEG_file(std::vector<unsigned char>& OutData, int32& OutWidth, int32& OutHeight, const char* InFilename)
{
    /* This struct contains the JPEG decompression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     */
    struct jpeg_decompress_struct cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    //struct my_error_mgr jerr;
    /* More stuff */
    FILE* infile;		/* source file */
    JSAMPARRAY buffer;		/* Output row buffer */
    int row_stride;		/* physical row width in output buffer */

    /* In this example we want to open the input file before doing anything else,
     * so that the setjmp() error recovery below can assume the file is open.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to read binary files.
     */

    fopen_s(&infile, InFilename, "rb");
    if (infile == NULL) {
        fprintf(stderr, "can't open %s\n", InFilename);
        return 0;
    }

    /* Step 1: allocate and initialize JPEG decompression object */

	LogErrorManager jerr;

    /* We set up the normal JPEG error routines, then override error_exit. */

	cinfo.err = jpeg_std_error((jpeg_error_mgr*)&jerr);
	jerr.Pub.error_exit = LogMessage;
	jerr.Pub.output_message = LogMessageDiscard;

    //jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.JumpBuffer))
	{
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return 0;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */

    jpeg_stdio_src(&cinfo, infile);

    /* Step 3: read file parameters with jpeg_read_header() */

    (void)jpeg_read_header(&cinfo, TRUE);
    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the stdio data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
     */

     /* Step 4: set parameters for decompression */

     /* In this example, we don't need to change any of the defaults set by
      * jpeg_read_header(), so we do nothing here.
      */

      /* Step 5: Start decompressor */

    (void)jpeg_start_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

     /* We may need to do some setup of our own at this point before reading
      * the data.  After jpeg_start_decompress() we have the correct scaled
      * output image dimensions available, as well as the output colormap
      * if we asked for color quantization.
      * In this example, we need to make an output work buffer of the right size.
      */
      /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    OutData.resize(cinfo.output_height * cinfo.output_width * 4);
	UINT newRowStride = cinfo.output_width * 4;
	UINT bpp = cinfo.output_components;

    OutWidth = cinfo.output_width;
    OutHeight = cinfo.output_height;

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     */
    while (cinfo.output_scanline < cinfo.output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);
        /* Assume put_scanline_someplace wants a pointer and sample count. */
        //put_scanline_someplace(buffer[0], row_stride);

		for (DWORD dw = 0; dw < cinfo.output_width; ++dw)
		{
			(&OutData[0] + newRowStride * (cinfo.output_scanline - 1) + dw * 4)[0] = (buffer[0] + dw * bpp)[0];
			(&OutData[0] + newRowStride * (cinfo.output_scanline - 1) + dw * 4)[1] = (buffer[0] + dw * bpp)[1];
			(&OutData[0] + newRowStride * (cinfo.output_scanline - 1) + dw * 4)[2] = (buffer[0] + dw * bpp)[2];
			(&OutData[0] + newRowStride * (cinfo.output_scanline - 1) + dw * 4)[3] = 255;		// Add alpha
		}
    }

    /* Step 7: Finish decompression */

    (void)jpeg_finish_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

     /* Step 8: Release JPEG decompression object */

     /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    /* After finish_decompress, we can close the input file.
     * Here we postpone it until after no more JPEG errors are possible,
     * so as to simplify the setjmp error logic above.  (Actually, I don't
     * think that jpeg_destroy can do an error exit, but why assume anything...)
     */
    fclose(infile);

    /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */

     /* And we're done! */
    return 1;
}

void jImageFileLoader::LoadTextureFromFile(jImageData& data, std::string const& filename, bool sRGB)
{
	if (std::string::npos != filename.find(".tga"))
	{
		// todo 
	}
	else if (std::string::npos != filename.find(".png"))
	{
		unsigned w, h;
		LodePNG::decode(data.ImageData, w, h, filename.c_str());
		data.Width = static_cast<int32>(w);
		data.Height = static_cast<int32>(h);
	}
	else if (std::string::npos != filename.find(".jpg") || std::string::npos != filename.find(".jpeg"))
	{
        read_JPEG_file(data.ImageData, data.Width, data.Height, filename.c_str());
	}
	data.Filename = filename;
	data.sRGB = sRGB;
}
