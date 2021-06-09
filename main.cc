#include "global.h"
#include "Enemy.h"
#include "EnemyBullet.h"
#include "LTexture.h"
#include "Dot.h"
#include "Bullet.h"

#include "Sensor.h"
#include "VisionEnemySensorLeft.h"
#include "VisionEnemySensorRight.h"
#include "VisionAllySensorLeft.h"
#include "VisionAllySensorRight.h"
#include "VisionDotBulletSensorLeft.h"
#include "VisionDotBulletSensorRight.h"

#include <fstream>

#include <functional>
#include <chrono>

const char *GEN_RES_FILE_NAME = "genRes.csv";
const char *FAVORITE_FILE_NAME = "genFavorite.txt";
const char *GENOME_FILE_NAME = "gen.txt";
const char *STATS_FILE_NAME = "stat.txt";

bool init();

bool loadMedia();

void close();

bool initialize_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO) == 0)
		return true;
	displayError("SDL could not initialize!", SDL_GetError());
	return false;
}

bool initialize_window()
{
	//Create window
	gWindow = SDL_CreateWindow( "GenA",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			SCREEN_WIDTH2, SCREEN_HEIGHT,
			SDL_WINDOW_SHOWN );
	if (gWindow != nullptr) return true;

	displayError("Window could not be created!", SDL_GetError());
	return false;
}

bool initialize_renderer() {
	//Create vsynced renderer for window
	// gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);
	if (gRenderer != nullptr) return true;

	displayError("Renderer could not be created!", SDL_GetError());
	return false;
}

bool initialize_sdl_image() {
	int imgFlags = IMG_INIT_PNG;
	if ( (IMG_Init(imgFlags) & imgFlags) == imgFlags) return true;

	displayError("SDL_image could not initialize!", IMG_GetError());
	return false;
}

bool initialize_sdl_ttf() {
	if ( TTF_Init() == 0) return true;

	displayError("SDL_ttf could not initialize!", TTF_GetError());
	return false;
}

bool init()
{
	if (not initialize_sdl()) return false;

	// Если не можем задать "хотелки" - это не смертельно.
	// Set texture filtering to linear
	if( not SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
		displayWarning( "Linear texture filtering not enabled!" );

	if (not initialize_window()) return false;
	if (not initialize_renderer()) return false;

	//Initialize renderer color
	SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

	if (not initialize_sdl_image()) return false;
	if (not initialize_sdl_ttf()) return false;

	return true;
}

bool loadMedia()
{
	if ( not gDotTexture.loadFromFile( "dot.bmp" ) ) return false;
	if ( not gBulletTexture.loadFromFile( "bullet.bmp" ) ) return false;
	if ( not gEnemyBulletTexture.loadFromFile( "bullet.bmp" ) ) return false;
	if ( not gEnemyTexture.loadFromFile( "enemy.bmp" ) ) return false;
	if ( not gBGTexture.loadFromFile( "bg.png" ) ) return false;
	if ( not gPanelTexture.loadFromFile( "panel.png" ) ) return false;

	//Open the font
	gFont = TTF_OpenFont( "lazy.ttf", 24 );
	if( gFont == nullptr ) {
		displayError("Failed to load lazy font!", TTF_GetError());
		return false;
	}

	return true;
}

void close()
{
	if (gRenderer) {
		SDL_DestroyRenderer( gRenderer );
		gRenderer = NULL;
	}

	if (gWindow) {
		SDL_DestroyWindow( gWindow );
		gWindow = NULL;
	}

	if (gFont) {
		TTF_CloseFont( gFont );
		gFont = NULL;
	}

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

struct СoefficientsAndMode
{
    std::vector<double> coef;
    int mode;

    СoefficientsAndMode(std::vector<double> c, double m) : coef(c), mode(m)
    {}
};



void writeStats(const std::vector<std::shared_ptr<Enemy>> &sortEnemy,
		const std::vector<int> &indices)
{
	std::ofstream out(STATS_FILE_NAME); // окрываем файл для записи
	if (not out.is_open()) return;

//	std::cout<<" запись произошла  "<< std::endl;
	for (int i = 0; i < NUMBEROFOPPONENTS; ++i) {
		out << " существо " << i << ", у которого количество попаданий по игроку= "<<sortEnemy[indices[i]]->getHittingTheDot()<<", а количество попаданий по союзнику="<<sortEnemy[indices[i]]->getHittingTheAlly()<<", а количество выстрелов= "<<sortEnemy[indices[i]]->getShotCount()<<", а количество движений  "<<sortEnemy[indices[i]]->getNumberOfMovements()<<", а время= "<<sortEnemy[indices[i]]->getTickCount()<<", а количество движений вниз="<<sortEnemy[indices[i]]->getNumberOfDown()<<", а количество пропущенных шагов "<<sortEnemy[indices[i]]->getStandMovements()<< std::endl;
		out << sortEnemy[indices[i]]->fitnessFunction() << std::endl;
	}
	out.close();
}

void writeGenome(std::vector<Genome> &genome, std::vector<int> &indices)
{
	std::ofstream out(GENOME_FILE_NAME); // окрываем файл для записи
	if (not out.is_open()) return;

//	std::cout<<" запись произошла  "<< std::endl;
	for (int n = 0; n < NUMBEROFOPPONENTS; ++n) {
		out <<" существо "<<n<<std::endl;
		for (unsigned i = 0; i < 2; ++i) {
			out <<" секция : "<<i<<std::endl;
			for (unsigned j = 0; j < genome[indices[0]].section_size(i); ++j) {
				out <<" "<<genome[indices[n]].operator ()(i, j)<<", ";
			}
			out <<"\n";
		}
	}
	out.close();
}

void writeFavorite(std::vector<std::shared_ptr<Enemy>> sortEnemy,
		std::vector<Genome> &genome, std::vector<int> &indices,
		int &generationCounter, double &favoriteGen)
{
	if (favoriteGen < sortEnemy[indices[0]]->fitnessFunction()){
		favoriteGen = sortEnemy[indices[0]]->fitnessFunction();
		std::ofstream out(FAVORITE_FILE_NAME); // окрываем файл для записи
		if (not out.is_open()) return;

//		std::cout<<" запись в genFavorite произошла на итерации "<<generationCounter<< std::endl;
		out <<"итерация: "<<generationCounter<<"\n существо, у которого количество попаданий по игроку= "<<sortEnemy[indices[0]]->getHittingTheDot()<<", а количество попаданий по союзнику="<<sortEnemy[indices[0]]->getHittingTheAlly()<<", а количество выстрелов= "<<sortEnemy[indices[0]]->getShotCount()<<", а количество движений  "<<sortEnemy[indices[0]]->getNumberOfMovements()<<", а время= "<<sortEnemy[indices[0]]->getTickCount()<< std::endl;
		out <<"Резульатат функции = "<<sortEnemy[indices[0]]->fitnessFunction()<< " ";
		out <<"\n";
		out <<" геном этого существа: "<<std::endl;
		for (unsigned i = 0; i < 2; ++i) {
			out <<" секция : "<<i<<std::endl;
			for (unsigned j = 0; j < genome[indices[0]].section_size(i); ++j) {
				out <<" "<<genome[indices[0]].operator ()(i, j)<<", ";
			}
			out <<"\n";
		}
		out.close();

	}
}

void writeGenRes(std::vector<std::shared_ptr<Enemy>> sortEnemy,
		std::vector<int> &indices, int &generationCounter,double &SumFF)
{
	std::ofstream out(GEN_RES_FILE_NAME, std::ios::app); // окрываем файл для записи
	if (not out.is_open()) return;

	out.imbue(std::locale(""));
	for (int i = 0; i < NUMBER_OF_ENEMY_IN_ONE_GROUP; ++i) {
		SumFF += sortEnemy[indices[i]]->fitnessFunction();
	}
	//						SumFF /= numberOfEnemyInOneGroup;
	out << generationCounter << ";" << SumFF / NUMBER_OF_ENEMY_IN_ONE_GROUP
			<< ";" << sortEnemy[indices[0]]->fitnessFunction() << ";"
			<< sortEnemy[indices[7]]->fitnessFunction() << std::endl;

	out.close();
}

void writeCoef(){
	СoefficientsAndMode cam = {{1.,2.,3.,4.,5.,6.,7.}, 1};
	std::ofstream camFileout("СoefficientsAndMode.txt");
//	if (not camFileout.is_open()) return;
	if (camFileout.is_open())
		{
		std::cout<<"SHTO YA NADELAL"<<std::endl;
			for (unsigned i = 0; i < cam.coef.size(); ++i) {
				camFileout <<cam.coef[i]<< " "<<std::endl;
			}
		}
	camFileout.close();
}

std::random_device random_device;
std::mt19937 engine{ random_device() };
std::uniform_int_distribution<> dist2(0, 9);
std::uniform_int_distribution<> forX(1, 620);
std::uniform_int_distribution<> forY(20, 80);

std::uniform_int_distribution<> forOrder(0, 254);
std::uniform_int_distribution<> forSplice(0, 7);

constexpr double Pmut = 0.02;
std::uniform_real_distribution<double> mut(0.0, 1.0);



int main( int argc, char* args[] )
{
	if ( not init() ) return 1;
	if ( not loadMedia()) return 2;

	bool quit = false;
	std::stringstream helthText;
	std::stringstream generationText;

	SDL_Color textColor = { 0, 0, 0, 255 };

	SDL_Event e;

	VisionEnemySensorLeft *visionEnemySensorLeft = new VisionEnemySensorLeft;
	VisionEnemySensorRight *visionEnemySensorRight = new VisionEnemySensorRight;
	VisionAllySensorLeft *visionAllySensorLeft = new VisionAllySensorLeft;
	VisionAllySensorRight *visionAllySensorRight = new VisionAllySensorRight;
	VisionDotBulletSensorLeft *visionDotBulletSensorLeft = new VisionDotBulletSensorLeft;
	VisionDotBulletSensorRight *visionDotBulletSensorRight = new VisionDotBulletSensorRight;

	Dot dot;
	Bullet bullet;


	std::vector<EnemyBullet> enemyBullet(NUMBEROFOPPONENTS);
	std::vector<Genome> genome(NUMBEROFOPPONENTS);
	std::vector<std::shared_ptr<Enemy>> enemy(NUMBEROFOPPONENTS);
	std::vector<int> enemyIdOnTheField;

	Enemy::SensorFunc s1, s2, s3, s4, s5, s6;
	Enemy::ActorFunc f1, f2, f3, f4, f5;


	int sec1Length = (enemy[1]->MAX_STATES);
	int sec2Length = (2<<enemy[1]->PREDICATE_COUNT)* (enemy[1]->MAX_STATES);

	std::vector<int> sec1(sec1Length);
	std::vector<int> sec2(sec2Length);

	writeCoef();

	for (int i = 0; i < NUMBEROFOPPONENTS; ++i) {
		for (int k = 0; k < sec1Length; ++k)
			sec1[k] =  dist2(random_device);
		for (int j = 0; j < sec2Length; ++j)
			sec2[j] =  dist2(random_device);

		genome[i].add_section(sec1);
		genome[i].add_section(sec2);  // 2^predic* states
		enemy[i] = std::make_shared<Enemy>(i, genome[i]);

		s1 = [&](unsigned id) -> double { return visionEnemySensorLeft->location((*enemy[id]), dot); };
		enemy[i] ->add_sensor(s1);
		s2 = [&](unsigned id) -> double { return visionEnemySensorRight->location((*enemy[id]), dot); };
		enemy[i] ->add_sensor(s2);
		s3 = [&](unsigned id) -> double { return visionDotBulletSensorLeft->location((*enemy[id]), bullet); };
		enemy[i] ->add_sensor(s3);
		s4 = [&](unsigned id) -> double { return visionDotBulletSensorRight->location((*enemy[id]), bullet); };
		enemy[i] ->add_sensor(s4);
		s5 = [&](unsigned id) -> double { return visionAllySensorLeft->location(*enemy[id], enemy, enemyIdOnTheField); };
		enemy[i] ->add_sensor(s5);
		s6 = [&](unsigned id) -> double { return visionAllySensorRight->location(*enemy[id], enemy, enemyIdOnTheField); };
		enemy[i] ->add_sensor(s6);

		f1 = [&](unsigned id){ enemy[id]->moveStraight(); };
		enemy[i]->add_actor(f1);
		f2 = [&](unsigned id){ enemy[id]->moveLeft(); };
		enemy[i]->add_actor(f2);
		f3 = [&](unsigned id){ enemy[id]->moveRight(); };
		enemy[i]->add_actor(f3);
		f4 = [&](unsigned id){ enemy[id]->moveBack(); };
		enemy[i]->add_actor(f4);
		f5 = [&, i](unsigned id){ enemy[id]->moveShot(enemyBullet[i]); };
		enemy[i]->add_actor(f5);

		enemy[i]->resetTickCount();
	}
	using clk = std::chrono::high_resolution_clock;
	auto start = clk::now();
	auto stop = start + std::chrono::seconds(TIME_OF_ONE_GENERATION);

	std::vector<std::shared_ptr<Enemy>> favoriteEnemy(8);
	std::vector<std::shared_ptr<Enemy>> sortEnemy(NUMBEROFOPPONENTS);
	std::vector<Genome> sortG(NUMBEROFOPPONENTS);
	std::vector<uint8_t>  order;
	std::vector<int> indices(NUMBEROFOPPONENTS);
	std::vector<bool> enemyOnTheFieldVector(NUMBEROFOPPONENTS);

	double favoriteGen = 0.;
	int enemyOnTheField = 0;
	int counterGroup = 1;
	int counterGroupGenome = 0;
	double SumFF = 0.;

	// Создание пустого файла
	{ std::ofstream empty_gen_res(GEN_RES_FILE_NAME); }

	int generationCounter=0;

	while( !quit )
	{
		while( SDL_PollEvent( &e ) != 0 )
		{
			if( e.type == SDL_QUIT )
			{
				quit = true;
			}
			dot.handleEvent( e );
			bullet.handleEvent(e, dot);
		}
		if (bullet.getMPosY() == 1000){
			bullet.setPosY(dot.getMPosY());
			bullet.setPosX(dot.getMPosX());
			bullet.setVelY(-5);
		}

		//Clear screen
		SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
		SDL_RenderClear( gRenderer );
		//Render background
		gBGTexture.render( 0, 0 );
		gPanelTexture.render( 640, 0 );

		helthText.str("");
		helthText << "health: " << dot.getHealth() ;
		generationText.str("");
		generationText << "generation: " << generationCounter;

		//Render text
		if( !gTextTexture.loadFromRenderedText( helthText.str().c_str(), textColor ) )
		{
			printf( "Unable to render time texture!\n" );
		}
		gTextTexture.render( 645, 50 );

		if( !gTextGenerationTexture.loadFromRenderedText( generationText.str().c_str(), textColor ) )
		{
			printf( "Unable to render time texture!\n" );
		}
		gTextGenerationTexture.render( 645, 20 );

		for (int i = 0; i < NUMBEROFOPPONENTS; ++i) {
			if (!(enemy[i] ->getDead()) && enemyOnTheField < SIMULTANEOUS_NUMBER_OF_ENEMY_ON_THE_FIELD && !enemy[i]->getEnemyOnTheField()){
					enemy[i] ->setEnemyOnTheField(true);
					enemy[i]->setMPosX(forX(random_device));
					enemy[i]->setMPosY(forY(random_device));
					enemyOnTheField++;
					enemyIdOnTheField.push_back(enemy[i]->getId());
//							std::cout<<" существ на поле:  "<<enemyOnTheField<<std::endl;
			}
		}
		bullet.move(dot);
		dot.move(enemy, enemyIdOnTheField, enemyBullet);
		bullet.render();
		dot.render();

		visionEnemySensorLeft->location(*enemy[enemyIdOnTheField[0]], dot);
		visionEnemySensorRight->location(*enemy[enemyIdOnTheField[0]], dot);
		visionAllySensorLeft->location(*enemy[enemyIdOnTheField[0]], enemy, enemyIdOnTheField);
		visionAllySensorRight->location(*enemy[enemyIdOnTheField[0]], enemy, enemyIdOnTheField);
		visionDotBulletSensorLeft->location(*enemy[enemyIdOnTheField[0]], bullet);
		visionDotBulletSensorRight->location(*enemy[enemyIdOnTheField[0]], bullet);
//				std::cout<<" координаты линии:  "<< visionEnemySensorLeft->bX<<" "<<visionEnemySensorLeft->bY<<" "<<visionEnemySensorLeft->centerEnemyPosX<<" "<<visionEnemySensorLeft->centerEnemyPosY<<std::endl;
		SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0x00);
		SDL_RenderDrawLine(gRenderer, visionEnemySensorLeft->bX, visionEnemySensorLeft->bY, visionEnemySensorLeft->centerEnemyPosX, visionEnemySensorLeft->centerEnemyPosY); // BA
		SDL_RenderDrawLine(gRenderer, visionEnemySensorLeft->cX, visionEnemySensorLeft->cY, visionEnemySensorLeft->centerEnemyPosX, visionEnemySensorLeft->centerEnemyPosY); // CA
		SDL_RenderDrawLine(gRenderer, visionEnemySensorLeft->cX, visionEnemySensorLeft->cY, visionEnemySensorLeft->bX, visionEnemySensorLeft->bY); // CB
		SDL_RenderDrawLine(gRenderer, visionEnemySensorLeft->kX, visionEnemySensorLeft->kY, visionEnemySensorLeft->centerEnemyPosX, visionEnemySensorLeft->centerEnemyPosY); // KA

		SDL_RenderDrawLine(gRenderer, visionDotBulletSensorLeft->kX, visionDotBulletSensorLeft->kY, visionDotBulletSensorLeft->centerEnemyPosX, visionDotBulletSensorLeft->centerEnemyPosY); // KA

		SDL_RenderDrawLine(gRenderer, visionEnemySensorRight->bX, visionEnemySensorRight->bY, visionEnemySensorRight->centerEnemyPosX, visionEnemySensorRight->centerEnemyPosY); // BA
		SDL_RenderDrawLine(gRenderer, visionEnemySensorRight->cX, visionEnemySensorRight->cY, visionEnemySensorRight->centerEnemyPosX, visionEnemySensorRight->centerEnemyPosY); // CA
		SDL_RenderDrawLine(gRenderer, visionEnemySensorRight->cX, visionEnemySensorRight->cY, visionEnemySensorRight->bX, visionEnemySensorRight->bY); // CB

//				SDL_RenderDrawLine(gRenderer, visionAllySensorLeft->kX, visionAllySensorLeft->kY, visionAllySensorLeft->centerEnemyPosX, visionAllySensorLeft->centerEnemyPosY); // KA
		for (int i = 0; i < SIMULTANEOUS_NUMBER_OF_ENEMY_ON_THE_FIELD; ++i) {
			enemy[enemyIdOnTheField[i]] ->tick();
			bullet.hittingTheEnemy(*enemy[enemyIdOnTheField[i]]);
			dot.hittingTheDot(enemyBullet[enemyIdOnTheField[i]], *enemy[enemyIdOnTheField[i]]);
			enemy[enemyIdOnTheField[i]] ->render();
			enemy[enemyIdOnTheField[i]] ->moveBull(enemyBullet[enemyIdOnTheField[i]]);
			enemyBullet[enemyIdOnTheField[i]].hittingTheBullet(bullet);

		}

		for (int i = 0; i < SIMULTANEOUS_NUMBER_OF_ENEMY_ON_THE_FIELD; ++i) {
			for (int j = 0; j < SIMULTANEOUS_NUMBER_OF_ENEMY_ON_THE_FIELD; ++j) {
				if(i != j)
					enemyBullet[enemyIdOnTheField[i]].hittingTheAlly(*enemy[enemyIdOnTheField[j]]);
			}
		}
		enemyOnTheField = 0;
		enemyIdOnTheField.clear();
		for (int j = 0; j < NUMBEROFOPPONENTS; ++j) {
			if (enemy[j]->getEnemyOnTheField()){
				enemyOnTheField++;
				enemyIdOnTheField.push_back(enemy[j]->getId());
			}
		}

		if (clk::now() >= stop){

			for (int i = 0; i < NUMBEROFOPPONENTS; ++i) {
//						std::cout<<" результат функции:  "<<enemy[i]->fitnessFunction()<<"  ";
				sortEnemy[i] = enemy[i];
			}

			for (unsigned i = 0; i < indices.size(); ++i) indices[i] = i;
			std::sort(std::begin(indices), std::end(indices), [&](int a, int b) -> int {
			  return sortEnemy[a]->fitnessFunction() > sortEnemy[b]->fitnessFunction();
			});
			writeStats(sortEnemy, indices);
			writeGenome(genome, indices);
			writeFavorite(sortEnemy, genome, indices, generationCounter, favoriteGen);
			writeGenRes(sortEnemy, indices, generationCounter, SumFF);

			order.resize(genome.size());
			for (unsigned i = 0; i < order.size(); ++i) {
				order[i] = forOrder(random_device);
			}
			for (int q = 0; q < (NUMBEROFOPPONENTS/NUMBER_OF_ENEMY_IN_ONE_GROUP); ++q) {

				for (int i = 1; i <= NUMBER_OF_ENEMY_IN_ONE_GROUP; ++i) {
					if(i == (NUMBER_OF_ENEMY_IN_ONE_GROUP * counterGroup)+1){
						counterGroup++;
						counterGroupGenome = 0;
					}
					genome[indices[i+NUMBER_OF_ENEMY_IN_ONE_GROUP*counterGroup-1 ]]=genome[indices[counterGroupGenome]];
					counterGroupGenome++;
				}
			}
			counterGroup = 1;
			counterGroupGenome = 0;
			for (int i = 1; i <= NUMBER_OF_ENEMY_IN_ONE_GROUP; ++i) {
				genome[indices[i+NUMBER_OF_ENEMY_IN_ONE_GROUP*counterGroup-1]] =  splice(genome[forSplice(random_device)], genome[forSplice(random_device)], order) ;
				genome[indices[i+NUMBER_OF_ENEMY_IN_ONE_GROUP*(counterGroup+2)-1]] =  splice(genome[forSplice(random_device)], genome[forSplice(random_device)], order);
				for (unsigned s = 0; s < 2; ++s) {
				  for (unsigned w = 0; w < genome[0].section_size(s); ++w) {
					for (unsigned b = 0; b < 4; ++b) {
					  if (mut(random_device) < Pmut) {
						genome[indices[i+NUMBER_OF_ENEMY_IN_ONE_GROUP*(counterGroup+1)-1]].mutate(s, w, b);
					  }
					}
				  }
				}
				for (unsigned s = 0; s < 2; ++s) {
				  for (unsigned w = 0; w < genome[0].section_size(s); ++w) {
					for (unsigned b = 0; b < 4; ++b) {
					  if (mut(random_device) < Pmut) {
						genome[indices[i+NUMBER_OF_ENEMY_IN_ONE_GROUP*(counterGroup+2)-1]].mutate(s, w, b);
					  }
					}
				  }
				}
				if (i == NUMBER_OF_ENEMY_IN_ONE_GROUP * counterGroup)
					counterGroup = 5;
			}
			counterGroup = 1;
			for (int k = 0; k < NUMBEROFOPPONENTS; ++k)
			{
				enemy[k]->setVelY(1);
				enemy[k]->setVelX(1);
				enemy[k]->resetTickCount();
				enemy[k]->resetShotCount();
				enemy[k]->resetNumberOfMovements();
				enemy[k]->resetTotalNumberOfMovements();
				enemy[k]->resetStandMovements();
				enemy[k]->resetNumberOfDown();
				enemy[k]->resetHittingTheAlly();
				enemy[k]->resetHittingTheDot();
				enemy[k]->setDead(false);
				enemy[k]->setMPosX(forX(random_device));
				enemy[k]->setMPosY(forY(random_device));
				enemy[k]->setEnemyOnTheField(false);
				enemyBullet[k].setPosX(-200);
				enemyBullet[k].setPosY(-200);

			}

			bullet.setPosY(1000);
			enemyOnTheField = 0;
			generationCounter++;
			SumFF = 0;
			dot.resetHealth();


//			std::cout<<"время прошло"<<std::endl;
			start = clk::now();
			stop = start + std::chrono::seconds(TIME_OF_ONE_GENERATION);
		}

		SDL_RenderPresent( gRenderer );
	}

	close();

	return 0;
}
