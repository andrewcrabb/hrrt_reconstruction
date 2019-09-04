#include "matrix_file.hpp"

int MatrixFile::read_bytes(void *where, int howmany) {
	fptr.read(data_pos, view_size);
	if (fptr.fail()) {
		LOG_ERROR("Failed reading fptr");
		return 1;
	}
	return 0;
}

int MatrixFile::set_read_position(int position) {
	fptr.seekg(position);
	return fptr.fail() ? 1 : 0;
}

int MatrixFile::set_header(interfile::Key const &key, std::string const &val) {
	interfile_header[key] = val;
	return 0;
}
