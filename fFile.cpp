/** Copyright (C) 2022 fafik77 ( https://github.com/fafik77/fFile-WinApi )
	fFile is a wrapper for WinApi methods (open, read, write) files

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see https://www.gnu.org/licenses/gpl-3.0.html or https://www.gnu.org/licenses/.
**/

#include "fFile.h"

bool fFile::open(const std::wstring& fileName_, DWORD dwDesiredAccess, DWORD dwCreationDisposition )
{
 //clear previous data
	this->close();
	this->dwDesiredAccess= dwDesiredAccess;
	this->dwCreationDisposition= dwCreationDisposition;
	this->fileName= fileName_;
	buffer.clear();
	buffer.reserve(bufferSize);
	eol.reset();
	filePosLast= 0;
 //open file
	this->fileHandle= CreateFileW(
		fileName_.c_str(),	//file name
		dwDesiredAccess,	//access
		(dwDesiredAccess&GENERIC_READ)? FILE_SHARE_READ: NULL,	//share (read)
		NULL,	//null
		dwCreationDisposition,	//create
		FILE_ATTRIBUTE_NORMAL,	//attribs
		NULL	//null
	);
 //get file info
	memset(&fileInfo, 0x00, sizeof(fileInfo) ); //zero out memory
	if(ok()){
		GetFileInformationByHandle(this->fileHandle, &this->fileInfo);
	}
	return ok();
}

size_t fFile::read(size_t amount, char* buff_Out)
{
	if(error()) return -1;
	if(!is_readable()) return -1;
	if(eof()) return -1;
	DWORD read= 0;
	ReadFile(this->fileHandle, buff_Out, amount, &read, NULL);
	return read;
}

size_t fFile::readLine(std::string& Line_Out)
{
	Line_Out.clear();
	if(error()) return -1;
	if(!is_readable()) return -1;

	fileoff filePosNow= tell();
	if(filePosNow!= filePosLast){
		buffer.clear();
	}
	 //get this file EOL
	if(eol.isDefault()){
		_getEolType(Line_Out);
		filePosLast= tell();
		return filePosLast.off.LowPart;
	}
	 //then check buffer content
	size_t readSize= 0;
	size_t readSizeTotal= 0;
	if( buffer.size() ){
		readSize= buffer.size();
	}
	else {
		buffer.resize(bufferSize);
		readSizeTotal+= readSize= read( bufferSize, &(buffer[0]) );
		filePosLast= tell();
		if(readSize!=std::string::npos)
			buffer.resize(readSize);
	}

	char newest2chars[2]= {0x00};
	size_t newlinePos= -1;
	while(readSize && readSize!=std::string::npos){
		 //check if last char of previous read was one of EOL
		if(Line_Out.size()){
			if( 0 == memcmp(newest2chars, eol.begin(), eol.size()) ){
				Line_Out.pop_back();
				buffer.erase(buffer.begin());
				return readSizeTotal;
			}
		}
		 //find EOL
		newlinePos= buffer.find( eol.begin(), 0, eol.size() );
		if( newlinePos!= buffer.npos ) {	//ok done
			Line_Out.append( buffer.begin(), buffer.begin()+ newlinePos ); //copy line to output
			buffer.erase( buffer.begin(), buffer.begin()+(newlinePos+ eol.size()) ); //remove line from buffer
			break;
		}
		else {	//repeat
			Line_Out.append( buffer ); //copy rest from buffer to output
			newest2chars[0]= buffer.back();
			buffer.resize(bufferSize);
			readSizeTotal+= readSize= read( bufferSize, &(buffer[0]) ); //read new batch
			filePosLast= tell();
			if(readSize!=std::string::npos)
				buffer.resize(readSize);
			newest2chars[1]= 0x00;
			if(buffer.size())
				newest2chars[1]= buffer.front();
		}
	}
	return readSizeTotal;
}
bool fFile::_getEolType(std::string& Line_Out)
{
	size_t readSize= 0;
	size_t newlinePos= 0;
	size_t reedTotal= 0;
	const char eolChars[]= {13,10};	//CR LF
	fafikLib_readLineByte_EOL foundEol;
	Line_Out.resize(bufferSize);
	reedTotal+= readSize= read( bufferSize, &(Line_Out[0]) ); //read new batch
	Line_Out.resize(reedTotal);

	while(readSize && readSize!=std::string::npos) {
		newlinePos= Line_Out.find_first_of( &eolChars[0], 0, 2 );
		if(newlinePos< Line_Out.size() ){ //found and not at end
			foundEol.setTo(Line_Out.at(newlinePos), Line_Out.at(newlinePos+1) );
			BYTE sized= 0;
			if(foundEol.begin()[0] == '\n'){	//LF
				if(foundEol.begin()[1] == 13)	//CR
					++sized;
			}
			else {
				if(foundEol.begin()[1] == '\n')	//CR LF
					++sized;
			}
			eol.setTo(foundEol.begin()[0], sized!=0 ? foundEol.begin()[1] : -1);

			buffer.clear();
			buffer.append(Line_Out.begin()+ newlinePos+ eol.size(), Line_Out.end()); //save buffer
			Line_Out.erase( Line_Out.begin()+ newlinePos, Line_Out.end()); //output line
			return true;
		}
		else { //repeat
			Line_Out.resize(reedTotal+ bufferSize);
			reedTotal+= readSize= read( bufferSize, &(Line_Out[reedTotal]) ); //read new batch
			Line_Out.resize(reedTotal);
		}
	}
	eol.setToType(fafikLib_readLineByte_EOL::Windows_EOL); //EOL not found, assume windows EOL
	return false;
}

size_t fFile::write(size_t amount, const char* buff_In)
{
	if(error()) return -1;
	if(!is_writable()) return -1;
	DWORD wrote= 0;
	WriteFile(this->fileHandle, buff_In, amount, &wrote, NULL);
	return wrote;
}
size_t fFile::writeNewLine()
{
	if(error()) return -1;
	if(!is_writable()) return -1;
	return write(eol.size(), eol.begin());
}

bool fFile::seek(fileoff offset, DWORD dwMoveMethod) //SetFilePointerEx(hand, offset, null, dw) (not 0 on succ)
{
	if(error()) return false;
	fileoff isAt;
	BOOL err= SetFilePointerEx(this->fileHandle, offset, &isAt.off, dwMoveMethod);
	return !err;
}
fileoff fFile::tell()const
{
	if(error()) return -1ll;
	fileoff isAt;
	SetFilePointerEx(this->fileHandle, fileoff(), &isAt.off, FILE_CURRENT);
	return isAt;
}
bool fFile::SetEndOfFile()
{
	if(error()) return false;
	if(is_writable()) return false;
	BOOL err= ::SetEndOfFile(this->fileHandle);
	return err;
}

