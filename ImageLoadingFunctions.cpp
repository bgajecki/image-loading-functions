//
// Created by Beniamin Gajecki
// 
#include <cstdio> // Operacje na pliku
#include <cmath> // abs
#include <intrin.h> // _byteswap_
#include "zlib/zlib.h" // uncompress

// GLuint LoadBmp(const char* filename)
unsigned char* LoadBmp(const char* filename)
{
	unsigned int dataPos, // Pozycja gdzie zaczynaj¹ siê dane
		imageSize,
		width, // Szerokoœæ
		height; // Wysokoœæ
	unsigned char* data, // Dane obrazu
		header[54]; // Nag³ówek

	FILE* file;
	fopen_s(&file, filename, "rb");

	if (file == NULL) return false;

	if (fread(header, 1, 54, file) != 54) // Wielkoœæ nag³ówka BMP wynosi 54 bajty
		return false;

	if (header[0] != 'B' || header[1] != 'M')
		return 0;

	dataPos = *(int*) & (header[0x0A]);
	imageSize = *(int*) & (header[0x22]);
	width = *(int*) & (header[0x12]);
	height = *(int*) & (header[0x16]);

	if (imageSize == 0)
		imageSize = width * height * 3; // RGB
	if (dataPos == 0)
		dataPos = 54; // Koniec nag³ówka 

	data = new unsigned char[imageSize]; // Tworze tablice o wielkoœci danych obrazu z pliku(bajty)
	fread(data, 1, imageSize, file);
	fclose(file);
	// Koniec pracy z plikiem

	// Zamiana danych na texture
	/*
	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	delete[] data;
	return texture;
	*/
	return data;
}

unsigned char PaethPredictor(unsigned char a, unsigned char b, unsigned char c)
{
	unsigned char p, pa, pb, pc;
	p = a + b - c;
	pa = abs(p - a);
	pb = abs(p - b);
	pc = abs(p - c);
	if (pa <= pb && pa <= pc)
		return a;
	else if (pb <= pc)
		return b;
	else
		return c;
}


// GLuint LoadPng(const char* filename)
unsigned char* LoadPng(const char* filename)
{
	unsigned long fSize,
		iDataPos, // Pozycja IDAT
		iDataSize, // Wielkoœæ IDAT
		uncompressedDataSize;
	unsigned int width, // Szerokoœæ 
		height, // Wysokoœæ
		bpp, // Bajty na piksel
		imageSize;
	unsigned char bitDeep, // G³êbia bitowa
		colorType; // Typ koloru
	unsigned char* fileData, 
		* compressedData, 
		* uncompressedData, 
		* imageData;

	FILE* file;
	fopen_s(&file, filename, "rb"); // Tryb - odczyt pliku binarnie
	if (file == NULL) return false;

	fseek(file, 0, SEEK_END); // Przesuwamy wskaŸnik pliku na koniec
	fSize = ftell(file); // Uzyskujemy wielkoœæ pliku poprzez wskaŸnik pliku
	rewind(file); // Ustawiamy wskaŸnik pliku na pocz¹tek

	fileData = new unsigned char[fSize]; // Tworze tablice o wielkoœci pliku(bajty)
	fread(fileData, 1, fSize, file); // Czytam ca³y plik
	fclose(file);
	// Koniec pracy z plikiem

	if (fileData[0] != 0x89 || fileData[1] != 'P' || fileData[2] != 'N' || fileData[3] != 'G')
		return false;

	// Wczytanie wartoœci z pliku
	width = _byteswap_ulong(*(int*) & (fileData[0x10])); // Konwersja litle endian na big endian
	height = _byteswap_ulong(*(int*) & (fileData[0x14]));
	bitDeep = fileData[0x18];
	colorType = fileData[0x19];
	for (iDataPos = 0; iDataPos < fSize; ++iDataPos)
		if (fileData[iDataPos] == 'I' && fileData[iDataPos + 1] == 'D' && fileData[iDataPos + 2] == 'A' && fileData[iDataPos + 3] == 'T')
			break;
	iDataSize = _byteswap_ulong(*(int*) & (fileData[iDataPos - 4]));

	switch (colorType)
	{
	case 0: // Skala szaroœci
		if (bitDeep == 16)
			bpp = 2;
		else
			bpp = 1;
		break;
	case 2: // RGB
		bpp = 3 * (bitDeep / 8);
		break;
	case 3: // Indeks palety i PLTE
		bpp = 2; // Tego nie jestem pewny
		break;
	case 4: // Skala szaroœci i próbka alfa
		bpp = 2 * (bitDeep / 8);
		break;
	case 6: // RGBA
		bpp = 4 * (bitDeep / 8);
		break;
	}
	imageSize = width * height * bpp; // Szerokoœæ * wysokoœæ * bajty na pixel

	compressedData = new unsigned char[iDataSize];
	for (unsigned long i = 0; i < iDataSize; ++i)
		compressedData[i] = fileData[i + iDataPos + 4]; // Wczytuje skompresowane dane
	delete[] fileData; // Usuwam tablice z danymi pliku

	uncompressedDataSize = imageSize + height; // Wielkoœæ zdjêcia(bajty) + kod filtru
	uncompressedData = new unsigned char[uncompressedDataSize];
	int nResult = uncompress(uncompressedData, &uncompressedDataSize, compressedData, iDataSize); // Dekompresuje dane

	if (nResult == Z_OK)
	{
		delete[] compressedData; // Usuwam tablice z kompresowanymi danymi
		imageData = new unsigned char[imageSize];
		for (unsigned long i = 0, k = 0; i < uncompressedDataSize; i += width * bpp + 1, ++k) // Wczytuje numer filtru danych
		{
			switch (uncompressedData[i]) // Filtr danych
			{
			case 0: // None
				for (unsigned long j = i; j < i + width * bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1];
				break;
			case 1: // Left - pobieranie wczeœniej wartoœci dannego kana³u i sumowanie
				for (unsigned long j = i; j < i + bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1];
				for (unsigned long j = i + bpp; j < i + width * bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1] + imageData[j - bpp];
				break;
			case 2: // Up - pobieranie wartoœci z wiersza znajduj¹cego siê u góry i sumowanie
				for (unsigned long j = i; j < i + width * bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1] + imageData[j - width * bpp];
				break;
			case 3: // Average - œrednia pomiêdzy metodami Up i Down zakr¹glona w dó³
				for (unsigned long j = i; j < i + bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1];
				for (unsigned long j = i + bpp; j < i + width * bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1] + ((imageData[j - bpp] + imageData[j - width * bpp]) / 2);
				break;
			case 4: // Paeth - szukanie najlepszej metody (a - left, b - up, c - left up)
				for (unsigned long j = i; j < i + bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1];
				for (unsigned long j = i + bpp; j < i + width * bpp; ++j)
					imageData[j - k] = uncompressedData[j + 1] + PaethPredictor(imageData[j - 4], imageData[j - width * bpp], imageData[j - width * bpp - bpp]);
				break;
			}
		}
		delete[] uncompressedData; // Usuwam odkompresowane dane, które s¹ przed filtracj¹

		// Zamiana danych na texture
		// Zak³adam, ¿e typ koloru 6(RGBA) i g³êbia bitowa 8 czyli jeden bajt na kana³
		/*
		GLuint texture;
		glGenTextures(1, &texture);

		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		delete[] imageData;
		return texture;
		*/
		return imageData;
	}
	delete[] uncompressedData;
	return false;
}
