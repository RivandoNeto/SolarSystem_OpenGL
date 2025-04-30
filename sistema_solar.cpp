#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <vector>

using namespace std;

// --- Variáveis globais ---
GLuint textureSun, textureLua,bgTexture;
GLUquadric* quadric;

const float RAIO_SKYBOX = 50.0f;
const float MARGEM_COLISAO = 0.5f; // Distância mínima da parede

float bgAngle = 0.0f;
float   anguloLua = 0.0f;     // ângulo de órbita da Lua
float   rotLua     = 0.0f;    // ângulo de rotação da Lua


int pause = 1;
float aux1[10], aux2[10];

// --- Estrutura dos Planetas ---
struct Planeta {
    const char* nome;
    float distanciaSol;
    float raio;
    float velocidadeOrbita;
    float velocidadeRotacao;
    GLuint textura;
    float anguloOrbita;
    float anguloRotacao;
};

vector<Planeta> planetas;

// Material para o efeito de emissão do Sol
GLfloat sunEmission[] = {1.0f, 0.9f, 0.3f, 1.0f};
GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};

// Parâmetros da luz
GLfloat sunLight[] = {1.0f, 0.94f, 0.5f, 1.0f};  // luz amarelada
GLfloat sunAmbient[] = {0.3f, 0.3f, 0.3f, 1.0f}; // luz ambiente suave

// Câmera
float eyeX = 0.0f, eyeY = 0.0f, eyeZ = 20.0f;
float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
float cameraSpeed = 0.5f;
float yaw = -90.0f;   // rotação em torno do eixo Y (olhar para a esquerda ou direita)
float pitch = 0.0f;   // rotação em torno do eixo X (olhar para cima ou para baixo)

// função para carregar textura via stb_image e gerar mipmaps com GLU ---
GLuint loadTexture(const char* filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Erro ao carregar textura: " << filename << std::endl;
        return 0;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Parâmetros de wrap e filtro
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Usa GLU para gerar toda a cadeia de mipmaps
    if (channels == 3) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else if (channels == 4) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else {
        // fallback: trata como RGB
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    }

    stbi_image_free(data);
    return texID;
}

// Função para verificar colisões
void verificarColisaoCamera() {
    // Calcula a distância do centro
    float distancia = sqrt(eyeX*eyeX + eyeY*eyeY + eyeZ*eyeZ);
    
    // Se ultrapassar o limite do skybox
    if(distancia > RAIO_SKYBOX - MARGEM_COLISAO) {
        // Normaliza o vetor posição e ajusta para o limite
        float fator = (RAIO_SKYBOX - MARGEM_COLISAO) / distancia;
        eyeX *= fator;
        eyeY *= fator;
        eyeZ *= fator;
    }
}

// --- Inicializar Planetas ---
void initPlanetas() {
    planetas = {
        // Nome        Dist  Raio VelOrb VelRot Textura        AngOrb AngRot
        {"Mercúrio",  4.0f, 0.5f, 1.6f, 0.1f, loadTexture("textures/mercury.jpg"), 0.0f, 0.0f},
        {"Vênus",     7.0f, 0.9f, 1.3f, 0.05f, loadTexture("textures/venus.jpg"),   0.0f, 0.0f},
        {"Terra",    10.0f, 1.0f, 1.0f, 1.0f, loadTexture("textures/earth.jpg"),   0.0f, 0.0f},
        {"Marte",    15.0f, 0.7f, 0.8f, 0.8f, loadTexture("textures/mars.jpg"),    0.0f, 0.0f},
        {"Júpiter", 20.0f, 2.5f, 0.4f, 2.5f, loadTexture("textures/jupiter.jpg"), 0.0f, 0.0f},
        {"Saturno", 25.0f, 2.0f, 0.3f, 2.2f, loadTexture("textures/saturn.jpg"),  0.0f, 0.0f},
        {"Urano",   28.0f, 1.5f, 0.2f, 1.8f, loadTexture("textures/uranus.jpg"),  0.0f, 0.0f},
        {"Netuno",  31.0f, 1.4f, 0.1f, 1.6f, loadTexture("textures/neptune.jpg"),0.0f, 0.0f}
    };
}

// --- Inicialização OpenGL ---
void init() {
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    bgTexture = loadTexture("textures/background.jpg");
    
    // Habilita iluminação
    glEnable(GL_LIGHTING);
    
    // Configura a luz (GL_LIGHT0)
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunLight);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);
    
    // Permite que as cores dos materiais sejam combinadas com a textura
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    // Configura modelo de iluminação para mostrar melhor o efeito da luz
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

        glMaterialfv(GL_FRONT, GL_SPECULAR, noEmission); // Desliga especular
        glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);      // Ajuste de brilho
        
    
    // Configura normais automáticas para objetos
    glEnable(GL_NORMALIZE);

    // Cria um quadric para desenhar esferas texturadas
    quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluQuadricNormals(quadric, GLU_SMOOTH);

    // Carrega as texturas JPEG
    textureSun     = loadTexture("textures/sun.jpg");
    textureLua  = loadTexture("textures/lua.jpg");


    quadric = gluNewQuadric();
    gluQuadricTexture(quadric, GL_TRUE);
    gluQuadricNormals(quadric, GLU_SMOOTH);

    initPlanetas();

}

void desenharOrbita(float raio) {
    glDisable(GL_LIGHTING); // Não queremos iluminação nas linhas
    glColor3f(1.0f, 1.0f, 1.0f); // Cor branca para as órbitas
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; ++i) {
        float theta = 2.0f * M_PI * i / 100;
        glVertex3f(cos(theta) * raio, 0.0f, sin(theta) * raio);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void desenharBackground() {
    glDisable(GL_LIGHTING); // Desliga iluminação
    glDisable(GL_DEPTH_TEST); // Ignora profundidade
    glEnable(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
        glRotatef(bgAngle, 0.0, 1.0, 0.0); // Rotação contínua
        glScalef(75.0f, 75.0f, 75.0f); // Tamanho da esfera
    
        GLUquadric* sphere = gluNewQuadric();
        gluQuadricTexture(sphere, GL_TRUE);
        gluQuadricOrientation(sphere, GLU_INSIDE); // Textura por dentro
        gluSphere(sphere, 1.0, 50, 50); // Desenha esfera invertida
        gluDeleteQuadric(sphere);
    
    glPopMatrix();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}


void timer(int value) {
    // Atualiza Lua
    anguloLua -= 0.2f;
    rotLua -= 0.04f;
    
    // Atualiza planetas
    for(auto& p : planetas) {
        p.anguloOrbita += 0.5f * p.velocidadeOrbita;
        p.anguloRotacao += 2.0f * p.velocidadeRotacao;
        
        if(p.anguloOrbita > 360.0f) p.anguloOrbita -= 360.0f;
        if(p.anguloRotacao > 360.0f) p.anguloRotacao -= 360.0f;
    }
    
    // Normaliza ângulos
    if(anguloLua >= 360.0f) anguloLua -= 360.0f;
    if(rotLua >= 360.0f) rotLua -= 360.0f;
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // 60 FPS
} 


// --- Função de desenho ---
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    desenharBackground();
    glLoadIdentity();

    // Posiciona a câmera
    gluLookAt(
        eyeX, eyeY, eyeZ,
        centerX, centerY, centerZ,
        0.0f, 1.0f, 0.0f
    );

    // --- Posiciona a luz no centro do Sol ---
    GLfloat lightPosition[] = {0.0f, 0.0f, 0.0f, 1.0f};  // Posição no centro (w=1 para fonte pontual)
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    // --- Desenha o Sol com textura ---
    glBindTexture(GL_TEXTURE_2D, textureSun);
    glPushMatrix();
        // O Sol emite luz própria (não é afetado por iluminação)
        glMaterialfv(GL_FRONT, GL_EMISSION, sunEmission);
        gluSphere(quadric, 3.5f, 50, 50);
        // Desliga a emissão de luz para os outros objetos
        glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
    glPopMatrix();

    // --- Desenha Mercúrio com textura ---
    glPushMatrix();

        // Planetas
        for(size_t i = 0; i < planetas.size(); ++i) {
            Planeta& p = planetas[i];

            if (i != 2){
                glPushMatrix();
                    // Órbita
                    glDisable(GL_TEXTURE_2D);
                    desenharOrbita(p.distanciaSol);
                    glEnable(GL_TEXTURE_2D);
        
                    // Transformações
                    glRotatef(p.anguloOrbita, 0.0f, 1.0f, 0.0f);
                    glTranslatef(p.distanciaSol, 0.0f, 0.0f);
                    glRotatef(p.anguloRotacao, 0.0f, 0.0f, 0.2f); // Muda o angulo de rotação dos planetas
        
                    // Planeta
                    glBindTexture(GL_TEXTURE_2D, p.textura);
                    gluSphere(quadric, p.raio, 40, 40);
                glPopMatrix();
            }
            //if (i == 2)
            else { // Caso Terra
                glPushMatrix();
                    // Órbita
                    glDisable(GL_TEXTURE_2D);
                    desenharOrbita(p.distanciaSol);
                    glEnable(GL_TEXTURE_2D);
            
                    // Transformações
                    glRotatef(p.anguloOrbita, 0.0f, 1.0f, 0.0f);
                    glTranslatef(p.distanciaSol, 0.0f, 0.0f);
            
                    // Desenha Terra
                    glBindTexture(GL_TEXTURE_2D, p.textura);
                    glPushMatrix();
                        glRotatef(p.anguloRotacao, 0.0f, 0.0f, 0.5f);
                        gluSphere(quadric, p.raio, 40, 40);
                    glPopMatrix();
                    
                    // Desenha Lua
                    glBindTexture(GL_TEXTURE_2D, textureLua);
                    glPushMatrix();
                        glRotatef(anguloLua, 0.0f, 0.0f, 1.0f);
                        glTranslatef(1.5f, 0.0f, 0.0f);
                        glRotatef(rotLua, 0.0f, 0.0f, 1.0f);
                        gluSphere(quadric, 0.2, 20, 20);
                    glPopMatrix();
            
                glPopMatrix();
            }
        }

        glPopMatrix();

    glutSwapBuffers();
}

// --- Ajusta viewport e projeção quando redimensionar janela ---
void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Ajuste na perspectiva para melhor visualização
    float aspect = (float)w / (float)h;
    gluPerspective(45.0f, aspect, 0.1f, 200.0f);  // Range maior para distâncias
    
    glMatrixMode(GL_MODELVIEW);
}

void updateCameraCenter() {
    // Converte ângulos para radianos
    float radYaw = yaw * M_PI / 180.0f;
    float radPitch = pitch * M_PI / 180.0f;

    // Calcula a direção da câmera (vetor frente)
    float frontX = cos(radPitch) * cos(radYaw);
    float frontY = sin(radPitch);
    float frontZ = cos(radPitch) * sin(radYaw);

    // Normaliza o vetor frente
    float length = sqrt(frontX*frontX + frontY*frontY + frontZ*frontZ);
    frontX /= length;
    frontY /= length;
    frontZ /= length;

    // Atualiza o ponto para onde a câmera está olhando
    centerX = eyeX + frontX;
    centerY = eyeY + frontY;
    centerZ = eyeZ + frontZ;
}

void teclado(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            exit(0);
       
        // Movimentação da câmera (andar)
        case 'w': {
            float dirX = centerX - eyeX;
            float dirY = centerY - eyeY;
            float dirZ = centerZ - eyeZ;
            eyeX += dirX * cameraSpeed;
            eyeY += dirY * cameraSpeed;
            eyeZ += dirZ * cameraSpeed;
            verificarColisaoCamera();
            updateCameraCenter();
            break;
        }
        case 's': {
            float dirX = centerX - eyeX;
            float dirY = centerY - eyeY;
            float dirZ = centerZ - eyeZ;
            eyeX -= dirX * cameraSpeed;
            eyeY -= dirY * cameraSpeed;
            eyeZ -= dirZ * cameraSpeed;
            verificarColisaoCamera();
            updateCameraCenter();
            break;
        }
        case 'a': {
            // Movimento lateral (esquerda)
            float radYaw = (yaw - 90.0f) * M_PI / 180.0f;
            eyeX += cos(radYaw) * cameraSpeed;
            eyeZ += sin(radYaw) * cameraSpeed;
            verificarColisaoCamera();
            updateCameraCenter();
            break;
        }
        case 'd': {
            // Movimento lateral (direita)
            float radYaw = (yaw + 90.0f) * M_PI / 180.0f;
            eyeX += cos(radYaw) * cameraSpeed;
            eyeZ += sin(radYaw) * cameraSpeed;
            verificarColisaoCamera();
            updateCameraCenter();
            break;
        }

        // Movimento vertical
        case ' ': // sobe
            eyeY += cameraSpeed;
            verificarColisaoCamera();
            updateCameraCenter();
            break;
        case 'c': // desce
            eyeY -= cameraSpeed;
            verificarColisaoCamera();
            updateCameraCenter();
            break;

        // Rotação da câmera
        case 'j': // gira para a esquerda
            yaw -= 5.0f;
            updateCameraCenter();
            break;
        case 'l': // gira para a direita
            yaw += 5.0f;
            updateCameraCenter();
            break;
        case 'i': // olha para cima
            pitch += 5.0f;
            if (pitch > 89.0f) pitch = 89.0f;
            updateCameraCenter();
            break;
        case 'k': // olha para baixo
            pitch -= 5.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            updateCameraCenter();
            break;
        case 'p': 
            if (pause == 1){
                for ( size_t i = 0; i < planetas.size(); i++){

                        aux1[i] = planetas[i].velocidadeOrbita;
                        aux2[i] = planetas[i].velocidadeRotacao;

                        planetas[i].velocidadeOrbita = 0;
                        planetas[i].velocidadeRotacao = 0;

                        glutPostRedisplay();
                }
                pause = 0;
            }
            else{
                for ( size_t i = 0; i < planetas.size(); i++){

                    planetas[i].velocidadeOrbita = aux1[i];
                    planetas[i].velocidadeRotacao = aux2[i];
                    glutPostRedisplay();
                }
                pause = 1;
            }
            break;
    }
    glutPostRedisplay();
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);  // Janela maior
    glutCreateWindow("Sistema Solar");

    init();
    
    // Configura callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(teclado);
    
    // Timer unificado
    glutTimerFunc(16, timer, 0);
    
    // Posição inicial da câmera
    eyeX = 0.0f;
    eyeY = 10.0f;
    eyeZ = 40.0f;
    updateCameraCenter();
    
    glutMainLoop();
    return 0;
}