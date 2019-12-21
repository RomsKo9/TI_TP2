#include <iostream>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <math.h>
#include <time.h>

struct circleToDetect {
  int r;
  int c;
  int rd;
  int score;
};

float calculDistance(int x1, int y1, int x2, int y2)
{
	return (sqrt(((x2-x1)*(x2-x1))+((y2-y1)*(y2-y1))));
}

float getprop(circleToDetect c)
{
	return (c.score / (2.*M_PI*c.rd));
}

void addBestCircles(std::vector<circleToDetect> *circleVector, circleToDetect c, int nb_circles)
{
	if(circleVector->size()<nb_circles)
	{
		circleVector->push_back(c);
	} else {
		float min = 99999;
		int idmin = -1;
		for(int i = 0; i < nb_circles; i++)
		{
			if(getprop(circleVector->at(i)) < min)
			{
				min = getprop(circleVector->at(i));
				idmin = i;
			}
		}
		if(idmin != -1 && getprop(c) > min)
		{
			circleVector->erase(circleVector->begin()+idmin);
			circleVector->push_back(c);
		}
	}
}

int main(int argc, char** argv ) {

  if( argc != 4)
   {
    std::cout <<" Usage: detect_cercle ImageToLoadAndDisplay nbCercleToDetect RadiusMini" << std::endl;
    return -1;
   }

   cv::Mat image, image_coins;
   image = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
   image_coins = cv::imread(argv[1], cv::IMREAD_COLOR);

   int nbCerclesToDetect = atoi(argv[2]);
   int radiusMiniToDetect = atoi(argv[3]);

   if(!image.data )
   {
       std::cout <<  "Could not open or find the image" << std::endl ;
       return -1;
   }

   float timeExec=0;
   clock_t t1,t2;



   std::cout << "###########################################" << '\n';
   std::cout << "#                                         #" << '\n';
   std::cout << "# Romain TESTOUD & Thibaud AVRIL-TERRONES #" << '\n';
   std::cout << "#                                         #" << '\n';
   std::cout << "###########################################" << '\n'<<'\n';
   std::cout << "Détection de cercles sur "<<argv[1]<<" en cours ..." << '\n'<<'\n';


  t1 = clock();
  //Filtre Gaussien
  cv::GaussianBlur(image, image, cv::Size(3,3), 0);

  //Sobel

  //Sobel pour x
  cv::Mat gradient_x;
  cv::Mat gradient_absolute_x;
  cv::Sobel(image, gradient_x, CV_16S, 1,0,3);
  cv::convertScaleAbs(gradient_x, gradient_absolute_x);
  //Sobel pour y
  cv::Mat gradient_y;
  cv::Mat gradient_absolute_y;
  cv::Sobel(image, gradient_y, CV_16S, 0,1,3);
  cv::convertScaleAbs(gradient_y, gradient_absolute_y);

  cv::Mat gradient;
  cv::addWeighted(gradient_absolute_x,0.5,gradient_absolute_y,0.5,0,gradient); // On fusionne les deux gradients

  // Création de l'accumulateur et accumulateurMax
	std::vector<std::vector<std::vector<uint>>> acc (image.rows, std::vector<std::vector<uint>>(image.cols, std::vector<uint>(calculDistance(0,0,image.rows, image.cols), 0)));
  std::vector<std::vector<std::vector<int>>> accMax (image.rows, std::vector<std::vector<int>>(image.cols, std::vector<int>(calculDistance(0,0,image.rows, image.cols), 0)));
  // Contour des cercles des images en niveaux de gris
  for(int r=0; r<gradient.rows;r++) {
    for(int c=0;c<gradient.cols;c++) {
      if (gradient.at<uchar>(r,c) > 150) {
        image.at<uchar>(r,c) = (uchar)150;
      	for(int i=0; i<gradient.rows; i++) {
      		for(int j=0; j<gradient.cols; j++) {
      			acc[i][j][(int)(calculDistance(i, j, r, c))]++;
      		}
      	}
      }
    }
  }

  // Calcule des maximums locaux
  // On parcourt lignes, col et rad de l'accumulateur

  for(int rA=1; rA<acc.size()-1; rA++) {
    for(int cA=1; cA<acc[0].size()-1; cA++) {
      for(int rdA=1; rdA<acc[0][0].size()-1; rdA++) {
        // On initialise le max local
        uint maxLocal=0;
        //on parcourt les cases voisines (27 voisins)
        for(int i=rA-1; i<rA+2; i++) {
					for(int j=cA-1; j<cA+2; j++) {
						for(int k=rdA-1; k<rdA+2; k++) {
							if(acc[i][j][k] > maxLocal) {
								maxLocal = acc[i][j][k];
              }
						}
					}
				}
        for(int i=rA-1; i<rA+2; i++)	{
					for(int j=cA-1; j<cA+2; j++) {
    				for(int k=rdA-1; k<rdA+2; k++) {
              if(acc[i][j][k] == maxLocal) {

                accMax[i][j][k]++;
              }
              else {
                accMax[i][j][k]--;
              }
            }
          }
        }
      }
    }
  }



  std::vector<circleToDetect> *circleWithHighestScore = new std::vector<circleToDetect>(0);

  for(int r = 0; r < acc.size(); r++) {
		for(int c = 0; c < acc[0].size(); c++) {
			for(int rd = 0; rd < acc[0][0].size(); rd++) {
				 if(accMax[r][c][rd]==27 && rd>radiusMiniToDetect) {
            circleToDetect cTd = {r,c,rd,acc[r][c][rd]};
            addBestCircles(circleWithHighestScore, cTd, nbCerclesToDetect);
				 }
			}
		}
	}

  for(int i=0; i<circleWithHighestScore->size() ; i++) {
    cv::circle(image_coins, cv::Point(circleWithHighestScore->at(i).c, circleWithHighestScore->at(i).r), circleWithHighestScore->at(i).rd, cv::Scalar(0,0,255));
		image_coins.at<cv::Vec3b>(circleWithHighestScore->at(i).r,circleWithHighestScore->at(i).c) = cv::Vec3b(0,255,0);

	}

  t2 = clock();
  timeExec = (float)(t2-t1)/CLOCKS_PER_SEC;

  std::cout << "\n################# RESULT #################" << '\n'<<'\n';
  std::cout << "Fichier chargé : "<<argv[1]<<'\n';
  std::cout << "Nombre de cercle(s) à détecter : "<<nbCerclesToDetect<<'\n';
  std::cout << "Rayon de détection minimal : "<<radiusMiniToDetect<<" px"<<'\n';
  std::cout << "------------------------" << '\n';
  std::cout << "Temps d'exécution : "<<timeExec<<" s" << '\n';
  std::cout << "------------------------" << '\n'<<'\n';


  cv::imshow( "Detection de Cercle", image_coins);

  cv::waitKey(0);
  return 0;
}
