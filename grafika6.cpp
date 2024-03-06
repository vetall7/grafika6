﻿#include <iostream>

#define GL3_PROTOTYPES 1
#include <glew.h>
#include <SDL.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#define WIDTH 800
#define HEIGHT 600

//------------------------------------------------------------------------------------------------------------------------------
//Globalne zmienne 

// kod zrodlowy shadera wierzcholkow
const GLchar* vertexSource =
"#version 150 core\n"
"in vec3 position;"
"in vec3 color;"
"in vec2 texcoord;"
"out vec3 Color;"
"out vec2 Texcoord;"
"uniform mat4 transformMatrix;"
"void main()"
"{"
"    gl_Position =  transformMatrix * vec4(position, 1.0);"
"    Color = color;"
"	 Texcoord = texcoord;"
"}";

// kod zrodlowy shadera fragmentow
const GLchar* fragmentSource =
"#version 150 core\n"
"in vec3 Color;"
"in vec2 Texcoord;"
"out vec4 outColor;"
"uniform sampler2D tex;"
"void main()"
"{"
"    outColor = texture(tex, Texcoord) * vec4(Color, 1.0);"
"}";


//------------------------------------------------------------------------------------------------------------------------------

GLint posAttrib, colAttrib, texAttrib;					//wskazniki atrybutow wierzcholkow
GLuint vertexShader, fragmentShader, shaderProgram;		//shadery


GLuint vao[5], vbo[14], ebo, tex;	// identyfikatory poszczegolnych obiektow (obiekty tablic wierzcholkow, buforow wierzcholkow, elementow, tekstury)

//------------------------------------------------------------------------------------------------------------------------------

GLfloat ver_floor[] = { //wspolrzedne wierzcholkow podlogi
	-5.0f,  0.0f, -5.0f,
	5.0f,  0.0f, -5.0f,
	5.0f,  0.0f,  5.0f,
	-5.0f,  0.0f,  5.0f,
};

GLfloat col_floor[] = { //kolory wierzcholkow podlogi
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
};

GLfloat tex_floor[] = {	//wspolrzedne tekstury dla podlogi, 
	//jesli wykraczaja poza przedzial <0,1> nastapi nakladanie zalezne od parametrow ustawionych za pomoca funkcji glTexParameteri, np. GL_REPEAT
0.0f, 0.0f,	// lewy dolny rog tekstury
5.0f, 0.0f,
5.0f, 5.0f,
0.0f, 5.0f, // prawy gorny rog tekstury
};


GLfloat ver_wall1[] = {
	0.0f, 0.0f, 3.0f,
	-5.0f, 0.0f, 3.0f,
	-5.0f, 5.0f, 3.0f,
	0.0f, 5.0f, 3.0f
};

GLfloat ver_wall2[] = {
	-2.0f, 0.0f, -2.0f,
	-2.0f, 0.0f, 3.0f,
	-2.0f, 5.0f, 3.0f,
	-2.0f, 5.0f, -2.0f
};

GLfloat ver_wall3[] = {
	5.0f, 0.0f, -1.0f,
	5.0f, 0.0f, 5.0f,
	5.0f, 5.0f, 5.0f,
	5.0f, 5.0f, -1.0f
};

GLfloat col_wall[] = { //kolory wierzcholkow podlogi
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
};


float pixels_wall[] = {
	0.0f, 1.0f, 0.8f,	0.2f, 2.0f, 8.0f,	1.0f, 0.3f, 0.0f,
	1.0f, 0.9f, 0.5f,	1.0f, 0.2f, 0.2f,	0.2f, 0.5f, 1.0f,
	1.0f, 0.6f, 0.5f,	0.0f, 0.0f, 1.0f,	1.0f, 0.2f, 0.4f
};



GLuint elements[] = { // pogrupowanie wierzcholkow w trojkaty, wykorzystane zarowno dla sciany jaki dla podlogi
	0,1,2,
	2,3,0,
};

float pixels_floor[] = { //tekstura o wymiarach 2x2; dla kazdego punktu okreslone skladowe RGB koloru 
	1.0f, 1.0f, 1.0f,   0.1f, 0.1f, 0.1f,
	0.1f, 0.1f, 0.1f,   1.0f, 1.0f, 1.0f,
};

GLfloat ver_triangle[] = { //wspolrzedne wierzcholkow trojkata okreslajacego polozenie obserwatora (kamery)
	-0.2f,  0.0f, 0.0f,
	 0.2f,  0.0f, 0.0f,
	 0.0f,  0.0f, -1.0f,
};

GLfloat col_triangle[] = { //kolory wierzcholkow trojkata okreslajacego polozenie obserwatora (kamery)
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
};


bool isInWall(float x, float z) {
	if (z >= -2.25 && z <= -3.25) return true; // sciana 1
	if (x <= -0.25f && x >= -1.0 && z >= -3.25 && z <= 1) return true; // sciana 2
	if (z >= -1 && (x >= 2.5f && x <= 2.75)) return true; // sciana 3
	return false;
}

//------------------------------------------------------------------------------------------------------------------------------

int init_shaders()
{
	// tworzenie i kompilacja shadera wierzcholkow
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cout << "Kompilacja shadera wierzcholkow NIE powiodla sie!\n";
		return 0;
	}

	// tworzenie i kompilacja shadera fragmentow
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		std::cout << "Kompilacja shadera fragmentow NIE powiodla sie!\n";
		return 0;
	}

	// dolaczenie programow przetwarzajacych wierzcholki i fragmenty do programu cieniujacego
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	// wskazniki atrybutow wierzcholkow
	posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);

	return 1;

}

//------------------------------------------------------------------------------------------------------------------------------

void create_objects()
{
	// generowanie obiektow
	glGenVertexArrays(5, vao);  // obiekt tablicy wierzcholkow, dla kazdego obiektu (np. dla podlogi) mamy jedna tablice
	glGenBuffers(14, vbo);		// obiekty buforow wierzcholkow, dla kazdego typu atrubutow kazdego obiektu mamy jeden bufor (np. bufor dla kolorow podlogi, bufor dla wspolrzednych podlogi itd.)
	glGenBuffers(1, &ebo);		// obiekt bufora elementow (ten sam bufor mozna wykorzystac zarowno dla podlogi jak i sciany)

	// podloga vao[0]

	glBindVertexArray(vao[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);	// bufor wspolrzednych wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_floor), ver_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);	// bufor kolorow wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_floor), col_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);	// bufor wspolrzednych tekstury podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_floor), tex_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// trojkat vao[1]

	//glBindVertexArray(vao[1]);

	//glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);	// bufor wspolrzednych wierzcholkow trojkata
	//glBufferData(GL_ARRAY_BUFFER, sizeof(ver_triangle), ver_triangle, GL_STATIC_DRAW);
	//glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glEnableVertexAttribArray(posAttrib);

	//glBindBuffer(GL_ARRAY_BUFFER, vbo[4]);	// bufor kolorow wierzcholkow trojkata
	//glBufferData(GL_ARRAY_BUFFER, sizeof(col_triangle), col_triangle, GL_STATIC_DRAW);
	//glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//glEnableVertexAttribArray(colAttrib);

	// sciana vao[2]

	glBindVertexArray(vao[2]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);	// bufor wspolrzednych wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_wall1), ver_wall1, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);	// bufor kolorow wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_wall), col_wall, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);	// bufor wspolrzednych tekstury podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_floor), tex_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// sciana vao[3]

	glBindVertexArray(vao[3]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[8]);	// bufor wspolrzednych wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_wall2), ver_wall2, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);	// bufor kolorow wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_wall), col_wall, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[10]);	// bufor wspolrzednych tekstury podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_floor), tex_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// sciana vao[4]

	glBindVertexArray(vao[4]);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[11]);	// bufor wspolrzednych wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(ver_wall3), ver_wall3, GL_STATIC_DRAW);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(posAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[12]);	// bufor kolorow wierzcholkow podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(col_wall), col_wall, GL_STATIC_DRAW);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(colAttrib);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[13]);	// bufor wspolrzednych tekstury podlogi
	glBufferData(GL_ARRAY_BUFFER, sizeof(tex_floor), tex_floor, GL_STATIC_DRAW);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(texAttrib);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

}

//------------------------------------------------------------------------------------------------------------------------------

void configure_texture()
{
	glGenTextures(1, &tex);		// obiekt tekstury
	glBindTexture(GL_TEXTURE_2D, tex);		// powiazanie tekstury z obiektem (wybor tekstury)

	// ustawienia parametrow tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// sposob nakladania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sposob filtrowania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels_floor); // ladowanie do tekstury tablicy pikseli
}


void configure_texture_wall1() {
	glGenTextures(1, &tex);		// obiekt tekstury
	glBindTexture(GL_TEXTURE_2D, tex);		// powiazanie tekstury z obiektem (wybor tekstury)

	// ustawienia parametrow tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// sposob nakladania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sposob filtrowania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 4, 0, GL_RGB, GL_FLOAT, pixels_wall); // ladowanie do tekstury tablicy pikseli
}

void configure_texture_wall2() {
	glGenTextures(1, &tex);		// obiekt tekstury
	glBindTexture(GL_TEXTURE_2D, tex);		// powiazanie tekstury z obiektem (wybor tekstury)

	// ustawienia parametrow tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// sposob nakladania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sposob filtrowania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels_wall); // ladowanie do tekstury tablicy pikseli
}


void configure_texture_wall3() {
	glGenTextures(1, &tex);		// obiekt tekstury
	glBindTexture(GL_TEXTURE_2D, tex);		// powiazanie tekstury z obiektem (wybor tekstury)

	// ustawienia parametrow tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	// sposob nakladania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // sposob filtrowania tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 4, 0, GL_RGB, GL_FLOAT, pixels_wall); // ladowanie do tekstury tablicy pikseli
}




int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_Window* window = SDL_CreateWindow("OpenGL", 100, 100, 800, 600, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_Event windowEvent;

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cout << "ERROR" << std::endl;
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // GL_ALWAYS)

	if (!init_shaders())
		return 0;

	create_objects();

	configure_texture();


	glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 20.0f);		 //macierz rzutowania perspektywicznego
	glm::mat4 viewMatrix;  //macierz widoku
	glm::mat4 transformMatrix; //macierz wynikowa

	GLint transformMatrixUniformLocation = glGetUniformLocation(shaderProgram, "transformMatrix");

	int top_view = false; //zmienna okreslajaca czy patrzymy na scene z gory

	glm::vec3 position = glm::vec3(0.0f, 1.0f, 0.0f); //poczatkowe polozenie kamery
	glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f); //poczatkowy kierunek, w ktorym kamera jest skierowana

	float kat = (1 * M_PI) / 60;
	float obecny = kat * 360;


	while (true)
	{
		if (SDL_PollEvent(&windowEvent))
		{
			if (windowEvent.type == SDL_KEYUP &&
				windowEvent.key.keysym.sym == SDLK_ESCAPE) break;
			if (windowEvent.type == SDL_QUIT) break;


			if (windowEvent.type == SDL_KEYDOWN)
			{
				glm::vec3 position_tmp = position;
				switch (windowEvent.key.keysym.sym)
				{
				case SDLK_SPACE:
					top_view = !top_view;
					break;

				case SDLK_UP:
					position_tmp.z += 0.25 * direction.z;
					position_tmp.x += 0.25 * direction.x;
					if (!isInWall(position_tmp.x, position_tmp.z)) {
						position = position_tmp;
						/*ver_triangle[0] += 0.25 * direction.x;
						ver_triangle[2] += 0.25 * direction.z;
						ver_triangle[3] += 0.25 * direction.x;
						ver_triangle[5] += 0.25 * direction.z;
						ver_triangle[6] += 0.25 * direction.x;
						ver_triangle[8] += 0.25 * direction.z;
						glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
						glBufferData(GL_ARRAY_BUFFER, sizeof(ver_triangle), ver_triangle, GL_STATIC_DRAW);*/

					}
					break;

				case SDLK_DOWN:
					position_tmp.z -= 0.25 * direction.z;
					position_tmp.x -= 0.25 * direction.x;
					if (!isInWall(position_tmp.x, position_tmp.z)) {
						position = position_tmp;
						/*ver_triangle[0] -= 0.25 * direction.x;
						ver_triangle[2] -= 0.25 * direction.z;
						ver_triangle[3] -= 0.25 * direction.x;
						ver_triangle[5] -= 0.25 * direction.z;
						ver_triangle[6] -= 0.25 * direction.x;
						ver_triangle[8] -= 0.25 * direction.z;
						glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
						glBufferData(GL_ARRAY_BUFFER, sizeof(ver_triangle), ver_triangle, GL_STATIC_DRAW);*/
					}
					break;
				case SDLK_LEFT:
					obecny -= kat;
					break;
				case SDLK_RIGHT:
					obecny += kat;
					break;

				}

			}

		}

		direction.x = cos(obecny);
		direction.z = sin(obecny);

		if (top_view) //patrzymy z gory
			viewMatrix = glm::lookAt(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));

		else //patrzymy z miejsca, w ktorym jest obserwator 
			viewMatrix = glm::lookAt(position, position + direction, glm::vec3(0.0f, 1.0f, 0.0f));


		transformMatrix = projectionMatrix * viewMatrix;				// wynikowa macierz transformacji
		glUniformMatrix4fv(transformMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(transformMatrix));	// macierz jako wejœciowa zmienna dla shadera wierzcholkow


		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);	// szare tlo
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		glBindVertexArray(vao[0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	//rysujemy podloge

		glBindVertexArray(vao[1]);
		glDisable(GL_TEXTURE_2D);
		glDrawArrays(GL_TRIANGLES, 0, 3);	//rysujemy trojkat przedstawiajacy polozenie kamery


		configure_texture_wall1();
		glBindVertexArray(vao[2]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	//rysujemy wall1

		configure_texture_wall2();
		glBindVertexArray(vao[3]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	//rysujemy wall1

		configure_texture_wall3();
		glBindVertexArray(vao[4]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);	//rysujemy wall3


		configure_texture();

		SDL_GL_SwapWindow(window);
	}

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(14, vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteTextures(1, &tex);
	glDeleteVertexArrays(5, vao);

	SDL_GL_DeleteContext(context);
	SDL_Quit();

	return 0;
}