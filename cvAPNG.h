#pragma once
#include "opencv2\opencv.hpp"

#include <vector>
#include <string>
#include <fstream>

class pngChunk{
private:
	class crcCalculator{
	private:
		unsigned long crcReg;
		unsigned long crcTable[256];
		void makeCRCTable(){
			for (int i = 0; i < 256; i++){
				unsigned long ul = (unsigned long)i;
				for (int j = 0; j < 8; j++){
					if (ul & 1)ul = 0xedb88320L ^ (ul >> 1);
					else ul = ul >> 1;
				}
				crcTable[i] = ul;
			}
		}
	public:
		void init(){
			crcReg = 0xffffffffL;
		}
		crcCalculator(){
			makeCRCTable();
			init();
		}
		void update(unsigned char uc){
			crcReg = crcTable[(crcReg ^ uc) & 0xff] ^ (crcReg >> 8);
		}
		void update(const std::vector<unsigned char>&ucvec){
			for (auto uc = ucvec.begin(); uc != ucvec.end(); uc++){
				update(*uc);
			}
		}
		unsigned long get(){
			return crcReg ^ 0xffffffffL;
		}
		void writeUCVec(std::vector<unsigned char>&ucvec){
			unsigned long ul = get();
			for (int i = 3; i >= 0; i--){
				unsigned char uc = 0;
				for (int j = 0; j < 8; j++){
					if (ul & (1L << (i * 8 + j))){
						uc |= (1L << j);
					}
				}
				ucvec.push_back(uc);
			}
		}
	};
	crcCalculator crcc;
	void calcCRC(){
		crc.clear();
		crcc.init();
		for (auto uc = chunkType.begin(); uc != chunkType.end(); uc++)crcc.update(*uc);
		crcc.update(chunkData);
		crcc.writeUCVec(crc);
	}
public:
	std::vector<unsigned char>buffer;
	int length;
	std::string chunkType;
	std::vector<unsigned char>chunkData;
	std::vector<unsigned char>crc;

	int readInt(const std::vector<unsigned char>&ucvec, int bytes, int idx, int &dst){
		dst = 0;
		for (int i = 0; i < bytes; i++){
			dst += (1 << (8 * i))*ucvec[idx + bytes - i - 1];
		}
		return idx + bytes;
	}
	int readString(const std::vector<unsigned char>&ucvec, int bytes, int idx, std::string &dst){
		dst = "";
		for (int i = 0; i < bytes; i++){
			dst += ucvec[idx + i];
		}
		return idx + bytes;
	}
	int readUCVec(const std::vector<unsigned char>&ucvec, int bytes, int idx, std::vector<unsigned char> &dst){
		dst.clear();
		for (int i = 0; i < bytes; i++){
			dst.push_back(ucvec[idx + i]);
		}
		return idx + bytes;
	}
	int readChunk(const std::vector<unsigned char>&ucvec, int idx){
		idx = readInt(ucvec, 4, idx, length);
		idx = readString(ucvec, 4, idx, chunkType);
		idx = readUCVec(ucvec, length, idx, chunkData);
		idx = readUCVec(ucvec, 4, idx, crc);
		return idx;
	}

	void writeInt(std::vector<unsigned char>&ucvec, int bytes, int src){
		for (int i = 0; i < bytes; i++){
			unsigned char uc = 0;
			for (int j = 0; j < 8;j++){
				if (src & (1L << (j + 8 * (bytes - i - 1)))){
					uc |= (1L << j);
				}
			}
			ucvec.push_back(uc);
		}
	}
	void writeString(std::vector<unsigned char>&ucvec, const std::string &src){
		for (auto uc = src.begin(); uc != src.end(); uc++){
			ucvec.push_back(*uc);
		}
	}
	void writeUCVec(std::vector<unsigned char>&ucvec, const std::vector<unsigned char> &src){
		for (auto uc = src.begin(); uc != src.end(); uc++){
			ucvec.push_back(*uc);
		}
	}
	void writeChunk(std::vector<unsigned char>&ucvec){
		length = (int)chunkData.size();
		writeInt(ucvec, 4, length);
		writeString(ucvec, chunkType);
		writeUCVec(ucvec, chunkData);
		calcCRC();
		writeUCVec(ucvec, crc);
	}

};

class cvAPNG{
private:
	std::ofstream ofs;
	void writeUC(unsigned char uc){
		ofs.write((const char *)&uc, sizeof(uc));
	}
	void writeUCVec(const std::vector<unsigned char>&ucvec){
		for (auto uc = ucvec.begin(); uc != ucvec.end(); uc++){
			writeUC(*uc);
		}
	}
	
	int imgWidth;
	int imgHeight;
	int imgFPS;

	std::vector<std::vector<unsigned char> >frames;

public:
	cvAPNG(int height = -1, int width = -1, int fps = 30){
		imgWidth = width;
		imgHeight = height;
		imgFPS = fps;
	}

	void push(const cv::Mat &img){
		if (imgWidth < 0 || imgHeight < 0){
			imgWidth = img.cols;
			imgHeight = img.rows;
		}
		cv::Mat cpy;
		img.copyTo(cpy);
		if (cpy.channels() == 1){
			cv::cvtColor(img, cpy, CV_GRAY2BGR);
		}
		cv::Mat tmpImg(imgHeight, imgWidth, CV_8UC3, cv::Scalar(255, 255, 255));
		cv::Mat txty = (cv::Mat_<double>(2, 3) << 1, 0, 0, 0, 1, 0);
		cv::warpAffine(cpy, tmpImg, txty, tmpImg.size(), CV_INTER_LINEAR, cv::BORDER_TRANSPARENT);
		std::vector<unsigned char>png;
		cv::imencode("img.png", tmpImg, png);
		pngChunk IDAT;
		IDAT.readChunk(png, 33);
		frames.push_back(IDAT.chunkData);
	}

	void write(std::string filename){
		ofs.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
		
		//signature
		writeUC(0x89);
		writeUC(0x50);
		writeUC(0x4E);
		writeUC(0x47);
		writeUC(0x0D);
		writeUC(0x0A);
		writeUC(0x1A);
		writeUC(0x0A);

		//IHDR
		pngChunk IHDR;
		IHDR.chunkType = "IHDR";
		IHDR.writeInt(IHDR.chunkData, 4, imgWidth);
		IHDR.writeInt(IHDR.chunkData, 4, imgHeight);
		IHDR.writeInt(IHDR.chunkData, 1, 8);
		IHDR.writeInt(IHDR.chunkData, 1, 2);
		IHDR.writeInt(IHDR.chunkData, 1, 0);
		IHDR.writeInt(IHDR.chunkData, 1, 0);
		IHDR.writeInt(IHDR.chunkData, 1, 0);
		IHDR.writeChunk(IHDR.buffer);
		writeUCVec(IHDR.buffer);

		//acTL
		pngChunk acTL;
		acTL.chunkType = "acTL";
		acTL.writeInt(acTL.chunkData, 4, (int)frames.size());
		acTL.writeInt(acTL.chunkData, 4, 0);
		acTL.writeChunk(acTL.buffer);
		writeUCVec(acTL.buffer);
	
		for (int i = 0; i < (int)frames.size(); i++){
			//fcTL
			pngChunk fcTL;
			fcTL.chunkType = "fcTL";
			fcTL.writeInt(fcTL.chunkData, 4, i == 0 ? 0 : i * 2 - 1);
			fcTL.writeInt(fcTL.chunkData, 4, imgWidth);
			fcTL.writeInt(fcTL.chunkData, 4, imgHeight);
			fcTL.writeInt(fcTL.chunkData, 4, 0);
			fcTL.writeInt(fcTL.chunkData, 4, 0);
			fcTL.writeInt(fcTL.chunkData, 2, 1);
			fcTL.writeInt(fcTL.chunkData, 2, imgFPS);
			fcTL.writeInt(fcTL.chunkData, 1, 0);
			fcTL.writeInt(fcTL.chunkData, 1, 0);
			fcTL.writeChunk(fcTL.buffer);
			writeUCVec(fcTL.buffer);
		
			if (i==0){
				//IDAT
				pngChunk IDAT;
				IDAT.chunkType = "IDAT";
				IDAT.chunkData = frames[i];
				IDAT.writeChunk(IDAT.buffer);
				writeUCVec(IDAT.buffer);
			}
			else{
				//fdAT
				pngChunk fdAT;
				fdAT.chunkType = "fdAT";
				fdAT.writeInt(fdAT.chunkData, 4, i * 2);
				for (auto uc = frames[i].begin(); uc != frames[i].end(); uc++){
					fdAT.chunkData.push_back(*uc);
				}
				fdAT.writeChunk(fdAT.buffer);
				writeUCVec(fdAT.buffer);
			}
		}
		
		//IEND
		pngChunk IEND;
		IEND.chunkType = "IEND";
		IEND.writeChunk(IEND.buffer);
		writeUCVec(IEND.buffer);
	}
};