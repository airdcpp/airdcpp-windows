/* 
 * Copyright (C) 2001-2024 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include <airdcpp/core/io/compress/BZUtils.h>

#include <airdcpp/core/classes/Exception.h>
#include <airdcpp/core/localization/ResourceManager.h>

namespace dcpp {
	
BZFilter::BZFilter() {
	memzero(&zs, sizeof(zs));

	if(BZ2_bzCompressInit(&zs, 9, 0, 30) != BZ_OK) {
		throw Exception(STRING(COMPRESSION_ERROR));
	}
}

BZFilter::~BZFilter() {
	dcdebug("BZFilter end, %u/%u = %.04f\n", zs.total_out_lo32, zs.total_in_lo32, (float)zs.total_out_lo32 / max((float)zs.total_in_lo32, (float)1)); 
	BZ2_bzCompressEnd(&zs);
}

bool BZFilter::operator()(const void* in, size_t& insize, void* out, size_t& outsize) {
	if(outsize == 0)
		return 0;

	zs.avail_in = insize;
	zs.next_in = (char*)in;
	zs.avail_out = outsize;
	zs.next_out = (char*)out;

	if(insize == 0) {
		int err = ::BZ2_bzCompress(&zs, BZ_FINISH);
		if(err != BZ_FINISH_OK && err != BZ_STREAM_END)
			throw Exception(STRING(COMPRESSION_ERROR));

		outsize = outsize - zs.avail_out;
		insize = insize - zs.avail_in;
		return err == BZ_FINISH_OK;
	} else {
		int err = ::BZ2_bzCompress(&zs, BZ_RUN);
		if(err != BZ_RUN_OK)
			throw Exception(STRING(COMPRESSION_ERROR));

		outsize = outsize - zs.avail_out;
		insize = insize - zs.avail_in;
		return true;
	}
}

UnBZFilter::UnBZFilter() {
	memzero(&zs, sizeof(zs));

	if(BZ2_bzDecompressInit(&zs, 0, 0) != BZ_OK) 
		throw Exception(STRING(DECOMPRESSION_ERROR));

}

UnBZFilter::~UnBZFilter() {
	dcdebug("UnBZFilter end, %u/%u = %.04f\n", zs.total_out_lo32, zs.total_in_lo32, (float)zs.total_out_lo32 / max((float)zs.total_in_lo32, (float)1)); 
	BZ2_bzDecompressEnd(&zs);
}

bool UnBZFilter::operator()(const void* in, size_t& insize, void* out, size_t& outsize) {
	if(outsize == 0)
		return 0;

	zs.avail_in = insize;
	zs.next_in = (char*)in;
	zs.avail_out = outsize;
	zs.next_out = (char*)out;

	int err = ::BZ2_bzDecompress(&zs);

	// No more input data, and inflate didn't think it has reached the end...
	if(insize == 0 && zs.avail_out != 0 && err != BZ_STREAM_END)
		throw Exception(STRING(DECOMPRESSION_ERROR));

	if(err != BZ_OK && err != BZ_STREAM_END)
		throw Exception(STRING(DECOMPRESSION_ERROR));

	outsize = outsize - zs.avail_out;
	insize = insize - zs.avail_in;
	return err == BZ_OK;
}

void BZUtil::decodeBZ2(const uint8_t* is, size_t sz, string& os) {
	bz_stream bs = { 0 };

	if(BZ2_bzDecompressInit(&bs, 0, 0) != BZ_OK)
		throw Exception(STRING(DECOMPRESSION_ERROR));

	// We assume that the files aren't compressed more than 2:1...if they are it'll work anyway,
	// but we'll have to do multiple passes...
	size_t bufsize = 2*sz;
	boost::scoped_array<char> buf(new char[bufsize]);

	bs.avail_in = sz;
	bs.avail_out = bufsize;
	bs.next_in = reinterpret_cast<char*>(const_cast<uint8_t*>(is));
	bs.next_out = &buf[0];

	int err;

	os.clear();

	while((err = BZ2_bzDecompress(&bs)) == BZ_OK) {
		if (bs.avail_in == 0 && bs.avail_out > 0) { // error: BZ_UNEXPECTED_EOF
			BZ2_bzDecompressEnd(&bs);
			throw Exception(STRING(DECOMPRESSION_ERROR));
		}
		os.append(&buf[0], bufsize-bs.avail_out);
		bs.avail_out = bufsize;
		bs.next_out = &buf[0];
	}

	if(err == BZ_STREAM_END)
		os.append(&buf[0], bufsize-bs.avail_out);

	BZ2_bzDecompressEnd(&bs);

	if(err < 0) {
		// This was a real error
		throw Exception(STRING(DECOMPRESSION_ERROR));
	}
}

} // namespace dcpp