#include <QImage>
#include <QFile>
#include <QTextStream>

#include <stdio.h>
#include <iostream>
#include <vector>

#include <math.h>

using namespace std;

void dumpImage(QString name, std::vector<float> &buffer, int width, int height) {
	float min = 1e20;
	float max = -1e20;
	for(float f: buffer) {
		min = std::min(f, min);
		max = std::max(f, max);
	}
	QImage img(width, height, QImage::Format_RGB32);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			float h = buffer[x + y*width];
			h = (h-min)/(max - min);
			img.setPixel(x, y, qRgb(255*h, 255*h, 255*h));
		}
	}
	img.save(name);
}

void dumpImage2(QString name, std::vector<float> &buffer, int width, int height, double cellsize) {
	float min = 1e20;
	float max = -1e20;
	for(float f: buffer) {
		min = std::min(f, min);
		max = std::max(f, max);
	}

	cout << "Min: " << min << " max:" << max << " steps: " << (max - min)/0.01 << endl;

	//highp float val = texture2d(samplerTop8bits, tex_coord) * (256.0 / 257.0);
	//val += texture2d(samplerBottom8bits, tex_coord) * (1.0 / 257.0);

	QImage img(width, height, QImage::Format_RGB32);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			float h = buffer[x + y*width];
			h = (h-min)/(max - min);
			//quantize to 65k
			int height = floor(h*65535);
			int r = height/256;
			int g = height%256;
			int b = 0;
			img.setPixel(x, y, qRgb(r, g, b));
		}
	}
	img.save(name);
	QFile info("info.json");
	if(!info.open(QFile::WriteOnly))
		throw "Failed writing info.json";

	QTextStream stream(&info);
	stream << "{\n"
	<< "width: " << width << ",\n"
	<< "heigh: " << height << ",\n"
	<< "type: \"dem\"\n"
	<< "cellsize: " << cellsize << ",\n"
	<< "min: " << min << ",\n"
	<< "max: " << max << "\n"
	<< "}\n";
}


using namespace std;
int main(int argc, char *argv[]) {
	FILE *fp = fopen(argv[1], "r");
	if(!fp) {
		cerr << "Could not open: " << argv[1] << endl;
		return 1;
	}
	int nrows, ncols;
	float xcenter, ycenter, cellsize, nodata;

	fscanf(fp, "ncols %d\n", &ncols);
	fscanf(fp, "nrows %d\n", &nrows);
	fscanf(fp, "xllcenter %f\n", &xcenter);
	fscanf(fp, "yllcenter %f\n", &ycenter);
	fscanf(fp, "cellsize %f\n", &cellsize);
	fscanf(fp, "nodata_value %f\n", &nodata);

	cout << nrows << " " << ncols << " " << xcenter << " " << ycenter << " " << cellsize << " " << nodata << endl;

	vector<float> heights(nrows*ncols);

	float min = 1e20;
	float max = -1e20;
	for(int i = 0; i < nrows*ncols; i++) {
		float h;
		fscanf(fp, "%f", &h);
		min = std::min(min, h);
		max = std::max(max, h);
		heights[i] = h;
	}



	int size = (nrows-2)*(ncols-2);
	vector<float> slopes(size);
	vector<float> aspects(size);

	for(int y = 1; y < nrows-1; y++) {
		for(int x = 1; x < ncols-1; x++) {

			/*			float m[3][3];
			for(int j = -1; j <= 1; j++)
				for(int j = -1; k <= 1; k++)
			float h = heights[x + ncols*y]; */


			int k = x + y*ncols;
			float a = heights[k -1 -ncols];
			float b = heights[k    -ncols];
			float c = heights[k +1 -ncols];
			float d = heights[k -1       ];
			float e = heights[k          ];
			float f = heights[k +1       ];
			float g = heights[k -1 +ncols];
			float h = heights[k    +ncols];
			float i = heights[k +1 +ncols];

			float rateOfChangeX = ((c + (2.0f*f) + i) - (a + (2.0f*d) + g)) / (8.0f * cellsize);
			float rateOfChangeY = ((g + (2.0f*h) + i) - (a + (2.0f*b) + c)) / (8.0f * cellsize);

			float slope = atan(sqrt(pow(rateOfChangeX,2.0) + pow(rateOfChangeY,2.0)));
			float aspect = atan2(rateOfChangeY, rateOfChangeX);
			slopes[x-1 + (y-1)*(ncols-2)] = slope;
			aspects[x-1 + (y-1)*(ncols-2)] = aspect;

		}
	}
	//dumpImage("slope.png", slopes, ncols-2, nrows-2);
	//dumpImage("aspect.png", aspects, ncols-2, nrows-2);
	dumpImage2("elevation.png", heights, ncols, nrows, cellsize);

	float zenith = M_PI*60.0f/180.0f;
	float azimuth = M_PI*-135.0f/180.0f;
	vector<float> hill(size);
	for(int y = 0; y < nrows-2; y++) {
		for(int x = 0; x < ncols-2; x++) {
			int k = x + y*(ncols-2);
			float s = slopes[k];
			float a = aspects[k];
			float shade = (cos(zenith) * cos(s)) + (sin(zenith) * sin(s) * cos(azimuth - a));
			hill[k] = shade;
		}
	}
	dumpImage("hillshade.png", hill, ncols-2, nrows-2);

	return 0;
}

