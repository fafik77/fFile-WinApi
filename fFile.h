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

#ifndef FFILE_H
#define FFILE_H

#include <string>

#include <windows.h>

 ///file offset
class fileoff
{
 public:
	fileoff() {off.QuadPart= 0ll;}
	fileoff(const DWORD LowPart, const LONG HighPart) {off.HighPart= HighPart; off.LowPart= LowPart;}
	fileoff(const LARGE_INTEGER val) {off= val;}
	fileoff(const LONGLONG val) {off.QuadPart= val;}

 	fileoff& operator = (const LONGLONG val) {off.QuadPart= val; return *this;}
 	fileoff& operator = (const LARGE_INTEGER& val) {off.QuadPart= val.QuadPart; return *this;}
 	operator LARGE_INTEGER&() {return off;}
 	operator LONGLONG() {return off.QuadPart;}

 	bool operator == (const LONGLONG val)const {return off.QuadPart== val;}
 	bool operator == (const fileoff& other)const {return off.QuadPart== other.off.QuadPart;}
 	bool operator > (const fileoff& other)const {return off.QuadPart> other.off.QuadPart;}
 	bool operator < (const fileoff& other)const {return off.QuadPart< other.off.QuadPart;}
 	bool operator >= (const fileoff& other)const {return off.QuadPart>= other.off.QuadPart;}
 	bool operator <= (const fileoff& other)const {return off.QuadPart<= other.off.QuadPart;}

 	fileoff& operator ++ () {++off.QuadPart; return *this;}
 	fileoff& operator += (const LONGLONG val) {off.QuadPart+= val; return *this;}
 	fileoff operator + (const LONGLONG val) { fileoff offs(*this); offs+= val; return offs;}
 	fileoff& operator -= (const LONGLONG val) {off.QuadPart-= val; return *this;}
 	fileoff operator - (const LONGLONG val) { fileoff offs(*this); offs-= val; return offs;}

 //store, data
	LARGE_INTEGER off;
};

class fFile{
 public:
 	 ///use open(...)
 	fFile(){}
 	~fFile() {close();}
 //struct
	 ///stores file EOL type
	struct fafikLib_readLineByte_EOL
	{
	 protected:
	  //data
			///0: -1 means auto determine & re-save here, 1: -1 means dont use
		char char_eol[3]= {-1, -1, 0};
		BYTE str_size : 7;
		BYTE str_NON_Default:1;
	 public:
	  //def
		enum fileLineEnd{
				///CR LF
			Windows_EOL= 1,
			DOS_EOL= 1,
			CRLF_EOL= 1,
				/// '\n' LF
			Unix_EOL= 2,
			GNU_Linux_EOL= 2,
			MacOsX_EOL= 2,
				/// '\r' CR
			MacOs_EOL= 3,
				/// '\0'
			Null_EOL= 0,
		};
	  //ctor
		 ///starts with default '\n' line end
		fafikLib_readLineByte_EOL(){ setToDefault('\n');}
		 ///starts with specified line end, and validates str_size
		fafikLib_readLineByte_EOL(char c1, char c2=-1){
			char_eol[0]= c1;
			char_eol[1]= c2;
			setSize(true);
		}
		void reset(){setToDefault('\n');}
	  //setter-functions
		 ///determine valid string size
		void setSize(bool forced=false){
		  if( (!str_size) || forced ){
			str_size= 0;
			if(char_eol[0]!= char(-1) ){
				++str_size;
				if(char_eol[1]!= char(-1) ){
					++str_size;
				} else
					char_eol[1]= -1;
			}
			if(!str_size){
				str_size= 1;
				char_eol[0]= '\n';
				char_eol[1]= -1;
				str_NON_Default= false;
			} else {
				str_NON_Default= true;
			}
		  }
		}
			///does validate str_size
		void setTo(char c1= -1, char c2= -1){
			char_eol[0]= c1;
			char_eol[1]= c2;
			setSize(true);
		}
			///adapts other & validates str_size
		void setTo(const fafikLib_readLineByte_EOL& other){
			char_eol[0]= other.char_eol[0];
			char_eol[1]= other.char_eol[1];
			setSize(true);
			str_NON_Default= other.str_NON_Default;
		}
		void setToType(fileLineEnd byType){
			if(byType==Windows_EOL){
				setTo(13,10);
			}else if(byType==Unix_EOL){
				setTo(10);
			}else if(byType==MacOs_EOL){
				setTo(13);
			}else if(byType==Null_EOL){
				setTo(0);
			}
			setSize(true);
		}

		fafikLib_readLineByte_EOL* operator = (const fafikLib_readLineByte_EOL& other){
			setTo(other);
			return this;
		}

	  //getters
		const char* getEolRaw()const {return char_eol+ 0;}
			///if written to use @a setSize(true)
		const char* begin()const {
			return char_eol+0 ;
		}
		 ///@param (wchar_t)output where to store the converted EOL, has to be at least 3 long
		 ///@return len of EOL string
		BYTE getAsWchar(wchar_t output[])const { if(!output)return -1; output[2]= 0; if(str_size) output[0]= char_eol[0]; if(str_size>1) output[1]= char_eol[1]; return str_size; }
		inline BYTE getSize()const { return size();}
		inline BYTE size()const {if(str_NON_Default) return str_size; else return 1;}
		inline BYTE realSize()const {return str_size;}
		inline bool isNonDefault()const {return str_NON_Default;}
		inline bool isDefault()const {return !str_NON_Default;}
		inline void setNonDefault() {str_NON_Default= true;}
	 protected:
		void setToDefault(char c1= -1, char c2= -1){
			char_eol[0]= c1;
			char_eol[1]= c2;
			str_size= 0;
			str_NON_Default= false;
		}
	};
 //function
	bool open(const std::wstring& fileName_, DWORD dwDesiredAccess= GENERIC_READ, DWORD dwCreationDisposition= OPEN_EXISTING );
	void close() { if(ok())CloseHandle(fileHandle); drop(); }
	void drop() {fileHandle= INVALID_HANDLE_VALUE;}

	 ///@return Bytes read or -1 on error
	size_t read(size_t amount, char* buff_Out);
	 ///@return Bytes read or -1 on error
	size_t readLine(std::string& Line_Out);
	size_t write(size_t amount, const char* buff_In);
	size_t writeNewLine();

	bool error()const {return fileHandle== INVALID_HANDLE_VALUE;}
	bool ok()const {return !error();}
	bool eof()const {return tell()>= size();}
	HANDLE getHandle()const {return fileHandle;}
	DWORD getAttrib()const {return fileInfo.dwFileAttributes;}
	std::wstring getName()const {return fileName;}
	FILETIME getCreationTime()const {return fileInfo.ftCreationTime;}
	BY_HANDLE_FILE_INFORMATION getInfo()const {return fileInfo;}
	fileoff size()const {return fileoff(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh); }
	fileoff getFilePosLastRLine()const {return filePosLast;}
	bool is_readable()const {return dwDesiredAccess& GENERIC_READ;}
	bool is_writable()const {return dwDesiredAccess& GENERIC_WRITE;}
	fafikLib_readLineByte_EOL getEol()const {return eol;}
	bool is_DefaultEol()const {return eol.isDefault();}
	void setEol(const fafikLib_readLineByte_EOL& to) {eol= to;}

	 ///@return 1= ok, 0= fail
	bool seek(fileoff offset, DWORD dwMoveMethod= FILE_BEGIN);
	fileoff tell()const;
	 ///@return 1= ok, 0= fail
	bool SetEndOfFile();
	 ///@return 1= ok, 0= fail
	bool SetEndOfFileAt(fileoff offset){ if(seek(offset)) return SetEndOfFile(); return false;}

 protected:
 //function
	 ///only used once by readLine()
	bool _getEolType(std::string& Line_Out);
 //store, data
	HANDLE fileHandle= INVALID_HANDLE_VALUE;
	BY_HANDLE_FILE_INFORMATION fileInfo;
	fafikLib_readLineByte_EOL eol;
	 ///used by readLine()
	fileoff filePosLast;
	DWORD dwDesiredAccess= 0;
	DWORD dwCreationDisposition= 0;
	std::wstring fileName;
	 ///4KB, changeable only in definition
	const int bufferSize= 4096;
	std::string buffer;
};

#endif // FFILE_H
