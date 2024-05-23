#include <windows.h>
#include <gl/glut.h>
#include <vector>
#include <cmath>
#include<winuser.h> 

using namespace std;

#ifndef WM_MOUSEWHEEL 
#define WM_MOUSEWHEEL 0x020A 
#endif 

// ��������
HDC hDC;
HGLRC hRC;
int Width, Height;

// ������ȯ�� ���� ����
int StartPt[2];

// ROTATE�� ���� ����
float Axis[3] = {1.0, 0.0, 0.0};
float Angle = 0.0;
float RotMat[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

// PAN �� ���� ����
float dT[3] = {0.0, 0.0, 0.0};

// ZOOM�� ���� ����
float ViewSize = 50.0;

float backWheelAngle = 0.0, armAngle = -20.0, frontWheelAngle = 0.0, shovelAngle = 10.0;

#define BUFFER_SIZE 64
GLuint SelectBuffer[BUFFER_SIZE];
#define BACKWHEELANGLE 10
#define ARMANGLE 11
#define FRONTWHEELANGLE 12
#define SHOVELANGLE 12

// �Լ�����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool SetupPixelFormat();
void DrawScene();
void SetupCamera();
void SetupViewport();
void SetupCameraProjection();

// ����� ���� �Լ�
void InitOpenGL();
void GetSphereCoord(int x, int y, float *px, float *py, float *pz);
void DrawFloor();
void SetMaterial(float r, float g, float b);
void Picking(int x, int y);
void DrawObject(GLenum mode);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// ������ Ŭ���� ���
	WNDCLASS wc;
	wc.style		= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	= (WNDPROC)WndProc;
	wc.cbClsExtra	= wc.cbWndExtra	= 0;
	wc.hInstance	= hInstance;
	wc.hIcon		= LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor		= LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName	 = NULL;
	wc.lpszClassName = "OpenGL Window Class";
	RegisterClass(&wc);

	// ������ ����
	HWND hWnd = CreateWindow("OpenGL Window Class", "3D SceneViewer", 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1008, 1027, 0, 0, hInstance, 0);

	// ������ ���
	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);

	// �޽��� ��������
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	switch (uMsg) 
	{
	case WM_KEYDOWN:
		{
			if (wParam == '1')
				backWheelAngle -= 2.0;
			if (wParam == '2')
				backWheelAngle += 2.0;

			if (wParam == '3')
				armAngle -= 2.0;
			if (wParam == '4')
				armAngle += 2.0;
			
			if (wParam == '5')
				frontWheelAngle -= 2.0;
			if (wParam == '6')
				frontWheelAngle += 2.0;

			if (wParam == '7')
				shovelAngle -= 2.0;
			if (wParam == '8')
				shovelAngle += 2.0;

			InvalidateRect(hWnd, NULL, false);
		}
	case WM_CREATE: 
		// ����̽����ؽ�Ʈ�� ����
		hDC = GetDC(hWnd);		
		if (!SetupPixelFormat()) 
			DestroyWindow(hWnd); 

		// ���������ؽ�Ʈ�� ����
		hRC = wglCreateContext(hDC); 
		wglMakeCurrent(hDC, hRC);

		// OpenGL �ɼ� �ʱ�ȭ
		InitOpenGL();
		break; 

	case WM_SIZE: 
		GetClientRect(hWnd, &rect);
		Width = rect.right;
		Height = rect.bottom;

		// ����Ʈ ��ȯ
		SetupViewport();
		InvalidateRect(hWnd, NULL, false);
		break; 

	case WM_DESTROY:
		if (hRC) 
			wglDeleteContext(hRC); 
		if (hDC) 
			ReleaseDC(hWnd, hDC); 
		PostQuitMessage(0);
		break;

	case WM_PAINT:
		// ���׸���
		DrawScene();
		ValidateRect(hWnd, NULL);
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		StartPt[0] = LOWORD(lParam);
		StartPt[1] = HIWORD(lParam);
		break;

	case WM_MOUSEWHEEL:
		{
			short int delta = HIWORD(wParam);
			delta /= 120;

			ViewSize += delta;
			if (ViewSize < 1.0)
				ViewSize = 1.0;

			InvalidateRect(hWnd, NULL, false);
		}
		break;
		
	case WM_MOUSEMOVE:
		if (!(wParam & MK_LBUTTON) && !(wParam & MK_RBUTTON))
			break;

		int x, y;
		x = LOWORD(lParam);
		y = HIWORD(lParam);

		if (wParam & MK_LBUTTON)
		{
			if (SelectBuffer[3] == 0) {
				float px, py, pz;
				float qx, qy, qz;

				GetSphereCoord(StartPt[0], StartPt[1], &px, &py, &pz);
				GetSphereCoord(x, y, &qx, &qy, &qz);

				Axis[0] = py * qz - pz * qy;
				Axis[1] = pz * qx - px * qz;
				Axis[2] = px * qy - py * qx;

				float len = Axis[0] * Axis[0] + Axis[1] * Axis[1] + Axis[2] * Axis[2];
				if (len < 0.0001)
				{
					Axis[0] = 1.0;
					Axis[1] = 0.0;
					Axis[2] = 0.0;
					Angle = 0.0;
				}
				else
				{
					Angle = px * qx + py * qy + pz * qz;
					Angle = acos(Angle) * 180.0f / 3.141592f;
				}
			}
			else if (SelectBuffer[3] == 10) {
				backWheelAngle -= -0.2 * (x - StartPt[0]);
			}
			else if (SelectBuffer[3] == 11) {
				frontWheelAngle -= -0.2 * (x - StartPt[0]);
			}
			else if (SelectBuffer[3] == 12) {
				armAngle -= -0.2 * (x - StartPt[0]);
			}
			else if (SelectBuffer[3] == 13) {
				shovelAngle -= -0.2 * (x - StartPt[0]);
			}
		}

		if (wParam & MK_RBUTTON)
		{
			dT[0] += 0.2 * (x - StartPt[0]);
			dT[1] += -0.2 * (y - StartPt[1]);

		}

		StartPt[0] = x;
		StartPt[1] = y;

		InvalidateRect(hWnd, NULL, false);
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		StartPt[0] = StartPt[1] = 0;
		Angle = 0.0;
		break;	
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void DrawObject(GLenum mode) {
	if (mode == GL_SELECT) {
		// �׽�Ʈ ��ü �׸���(�ҵ���)

	// �⺻ ����(���)
		SetMaterial(1.0, 1.0, 0.0);
		// Ʈ�� Ʈ��ũ
		glPushMatrix();
			glTranslatef(-5.0, 12.0, 0.0);
			glScalef(15.0, 14.0, 15.0);
			glutSolidCube(1.0);
		glPopMatrix();

		// Ʈ�� �Ӹ�
		glPushMatrix();
			// �ܺ�
			glTranslatef(6.0, 11.0, 0.0);
			glScalef(8.0, 12.0, 15.0);
			glutSolidCube(1.0);

			// ����(�ϴû�)
			glTranslatef(0.6, 0.1, 0.0);
			glScalef(0.3, 0.75, 0.9);
			SetMaterial(0.0, 3.5, 3.5);
			glutSolidCube(1.0);

			glTranslatef(0.0, -0.6, 0.0);
			glScalef(1.5, 0.4, 1.1);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
		glPopMatrix();

		//����(£�� ȸ��)
		// angle2�� ���� ȸ��
		glPushMatrix();
		SetMaterial(0.1, 0.1, 0.1);
		// �� ����
		glPushMatrix();
		glTranslatef(7.0, 5.0, 0);
		glScalef(1.0, 1.0, 1.0);
		// ���� ��
		glPushMatrix();
		glTranslatef(0.0, 0.0, 7.5);
		glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// ��
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(1.0);
		glPopMatrix();

		// ���� ��
		glPushMatrix();
		glTranslatef(0.0, 0.0, -7.5);
		glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// ��
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(1.0);
		glPopMatrix();
		glPopMatrix();
		glPopMatrix();

		// angle0�� ���� ȸ��
		glPushMatrix();
		SetMaterial(0.1, 0.1, 0.1);
		glTranslatef(-7.0, 5.0, 0.0);
		glRotatef(backWheelAngle, 0.0, 0.0, 1.0);
		// �� ���� ��(����)
		glPushMatrix();
		glTranslatef(0.0, 0.0, 7.5);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// ��(�ε� ȸ���ϴ� ���� Ȯ���ϱ� ���� ���� �� ũ�� ���)
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(0.3);
		glPopMatrix();

		// �� ���� ��(����)
		glPushMatrix();
		glTranslatef(0.0, 0.0, -7.5);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// ��
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(1.0);
		glPopMatrix();
		glPopMatrix();

		// angle1�� ���� ȸ��
		glPushMatrix();
		glTranslatef(5.0, 10.0, 0.0);
		glRotatef(armAngle, 0.0, 0.0, 1.0);
		// �� ��(����)
		glPushMatrix();
		glTranslatef(0.0, 0.0, 8.5);
		glScalef(2.0, 2.0, 2.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		// �ȶ� �߰�
		glTranslatef(4.0, 0.0, 0.0);
		glScalef(7.5, 1.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();

		// �� ��(����)
		glPushMatrix();
		glTranslatef(0.0, 0.0, -8.5);
		glScalef(2.0, 2.0, 2.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		// �ȶ� �߰�
		glTranslatef(4.0, 0.0, 0.0);
		glScalef(7.5, 1.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();


		// angle3�� ���� ȸ��
		glPushMatrix();
		glTranslatef(15.0, 0.0, 0.0);
		glRotatef(shovelAngle, 0.0, 0.0, 1.0);
		// ��(����)
		glScalef(1.5, 1.2, 1.0);
		glPushMatrix();
		glTranslatef(0.0, 0.0, 0.0);
		glScalef(1.0, 1.0, 15.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		// �� (£�� ȸ��)
		// ����
		glPushMatrix();
		glTranslatef(4.0, 0.0, 0.0);
		glScalef(7.0, 1.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();
		// ����
		glPushMatrix();
		glTranslatef(0.0, 3.5, 0.0);
		glScalef(1.0, 6.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();

		// �ܺ�(���� Ȯ���� ���� ���� ���� ����)
		glScalef(1.0, 1.0, 1.0 / 15.0); // ������ ����
		glPushMatrix();
		glTranslatef(3.5, 3.5, -7.0);
		glScalef(8.0, 6.0, 1.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		glPopMatrix();

		glPopMatrix();
		glPopMatrix();
		glPopMatrix();
	}
	else {
		// �׽�Ʈ ��ü �׸���(�ҵ���)

			// �⺻ ����(���)
			SetMaterial(1.0, 1.0, 0.0);
			// Ʈ�� Ʈ��ũ
			glPushMatrix();
			glTranslatef(-5.0, 12.0, 0.0);
			glScalef(15.0, 14.0, 15.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// Ʈ�� �Ӹ�
			glPushMatrix();
			// �ܺ�
			glTranslatef(6.0, 11.0, 0.0);
			glScalef(8.0, 12.0, 15.0);
			glutSolidCube(1.0);

			// ����(�ϴû�)
			glTranslatef(0.6, 0.1, 0.0);
			glScalef(0.3, 0.75, 0.9);
			SetMaterial(0.0, 3.5, 3.5);
			glutSolidCube(1.0);

			glTranslatef(0.0, -0.6, 0.0);
			glScalef(1.5, 0.4, 1.1);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();

			//����(£�� ȸ��)
			// angle2�� ���� ȸ��
			glPushMatrix();
			SetMaterial(0.1, 0.1, 0.1);
			// �� ����
			glPushMatrix();
			glTranslatef(7.0, 5.0, 0);
			glScalef(1.0, 1.0, 1.0);
			// ���� ��
			glPushMatrix();
			glTranslatef(0.0, 0.0, 7.5);
			glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// ��
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// ���� ��
			glPushMatrix();
			glTranslatef(0.0, 0.0, -7.5);
			glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// ��
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(1.0);
			glPopMatrix();
			glPopMatrix();
			glPopMatrix();

			// angle0�� ���� ȸ��
			glPushMatrix();
			SetMaterial(0.1, 0.1, 0.1);
			glTranslatef(-7.0, 5.0, 0.0);
			glRotatef(backWheelAngle, 0.0, 0.0, 1.0);
			// �� ���� ��(����)
			glPushMatrix();
			glTranslatef(0.0, 0.0, 7.5);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// ��(�ε� ȸ���ϴ� ���� Ȯ���ϱ� ���� ���� �� ũ�� ���)
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(0.3);
			glPopMatrix();

			// �� ���� ��(����)
			glPushMatrix();
			glTranslatef(0.0, 0.0, -7.5);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// ��
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(1.0);
			glPopMatrix();
			glPopMatrix();

			// angle1�� ���� ȸ��
			glPushMatrix();
			glTranslatef(5.0, 10.0, 0.0);
			glRotatef(armAngle, 0.0, 0.0, 1.0);
			// �� ��(����)
			glPushMatrix();
			glTranslatef(0.0, 0.0, 8.5);
			glScalef(2.0, 2.0, 2.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			// �ȶ� �߰�
			glTranslatef(4.0, 0.0, 0.0);
			glScalef(7.5, 1.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// �� ��(����)
			glPushMatrix();
			glTranslatef(0.0, 0.0, -8.5);
			glScalef(2.0, 2.0, 2.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			// �ȶ� �߰�
			glTranslatef(4.0, 0.0, 0.0);
			glScalef(7.5, 1.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();


			// angle3�� ���� ȸ��
			glPushMatrix();
			glTranslatef(15.0, 0.0, 0.0);
			glRotatef(shovelAngle, 0.0, 0.0, 1.0);
			// ��(����)
			glScalef(1.5, 1.2, 1.0);
			glPushMatrix();
			glTranslatef(0.0, 0.0, 0.0);
			glScalef(1.0, 1.0, 15.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			// �� (£�� ȸ��)
			// ����
			glPushMatrix();
			glTranslatef(4.0, 0.0, 0.0);
			glScalef(7.0, 1.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();
			// ����
			glPushMatrix();
			glTranslatef(0.0, 3.5, 0.0);
			glScalef(1.0, 6.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// �ܺ�(���� Ȯ���� ���� ���� ���� ����)
			glScalef(1.0, 1.0, 1.0 / 15.0); // ������ ����
			glPushMatrix();
			glTranslatef(3.5, 3.5, -7.0);
			glScalef(8.0, 6.0, 1.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			glPopMatrix();

			glPopMatrix();
			glPopMatrix();
			glPopMatrix();
	}
}

void Picking(int x, int y)
{
	int result, viewport[4];

	// 1. ���ù��� ����
	memset(SelectBuffer, 0, BUFFER_SIZE);
	glSelectBuffer(BUFFER_SIZE, SelectBuffer);

	// 2. ������ ��� ����
	glRenderMode(GL_SELECT);

	// 3. ���ӽ��� �ʱ�ȭ �� 0�Է�
	glInitNames();
	glPushName(0);

	// 4. ������� ���� �� ���� ������� ���� ��, ������� ����
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// 5. ����Ʈ ��ȯ ������ ���, ������ ���� ������� ���� �� ���� ���(���� ���)�� ���ϱ�
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluPickMatrix((double)x, (double)(viewport[3] - y), 2.0, 2.0, viewport);

	// 6. ������ȯ ��� ���� �� ���� ��Ŀ� ���ϱ�
	glOrtho(-ViewSize, ViewSize, -ViewSize, ViewSize, -1000.0f, 1000.0f);


	// 7. ���̵� ���ӽ��ÿ� �����ϸ鼭 ��ü�׸���
	DrawObject(GL_SELECT);

	// 8. ���� ��� ���� �� ���� ������� ����
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// 9. ������ ��� ����, ���õ� ��ü�� ���� ��ȯ
	result = glRenderMode(GL_RENDER);
}

bool SetupPixelFormat() 
{ 
	PIXELFORMATDESCRIPTOR pfd; 
	int pixelformat; 

	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); 
	pfd.nVersion = 1; 
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; 
	pfd.dwLayerMask = PFD_MAIN_PLANE; 
	pfd.iPixelType = PFD_TYPE_RGBA; 
	pfd.cColorBits = 24;
	pfd.cDepthBits = 16; 
	pfd.cAccumBits = 0; 
	pfd.cStencilBits = 0;

	if ((pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0) 
	{ 
		MessageBox(NULL, "ChoosePixelFormat() failed!!!", "Error", MB_OK|MB_ICONERROR); 
		return false; 
	} 

	if (SetPixelFormat(hDC, pixelformat, &pfd) == false) 
	{ 
		MessageBox(NULL, "SetPixelFormat() failed!!!", "Error", MB_OK|MB_ICONERROR); 
		return false; 
	}

	return true; 
}

void InitOpenGL()
{
	// OpenGL �ʱ�ȭ
	glEnable(GL_DEPTH_TEST);

	// Lighting.
	glEnable(GL_LIGHTING);

	// Back-face culling
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_LINE);

	// Shading
	glShadeModel(GL_SMOOTH);

	GLfloat light_position0[4] = { 0.0f, 0.0f, 10000.0f, 0.0f };
	GLfloat light_ambient0[] = { 0.5f, 0.5f, 0.5f };
	GLfloat light_diffuse0[] = { 0.5f, 0.5f, 0.5f };
	GLfloat light_specular0[] = { 0.8f, 0.8f, 0.8f };

	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular0);

	glEnable(GL_LIGHT0);
}

void SetupViewport()
{ 
	glViewport(0, 0, Width, Height);
}


void SetupCamera()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// ī�޶� ��ġ �ڷ� ����
	glTranslatef(0.0f, 0.0f, -10.0f);
	glTranslatef(dT[0], dT[1], dT[2]);

	// ������ǥ�� ȸ��
	glRotatef(Angle, Axis[0], Axis[1], Axis[2]);
	glMultMatrixf(RotMat);
	glGetFloatv(GL_MODELVIEW_MATRIX, RotMat);
	RotMat[12] = RotMat[13] = RotMat[14] = 0.0;
}

void SetupCameraProjection()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// �������� ��ļ���
	glOrtho(-ViewSize, ViewSize, -ViewSize, ViewSize, -1000.0f, 1000.0f);
}

void SetMaterial(float r, float g, float b)
{
	GLfloat mat_ambient[4];
	GLfloat mat_diffuse[4];
	GLfloat mat_specular[4] = { 0.9f, 0.9f, 0.9f, 0.6f };
	GLfloat mat_shininess[1] = {128.0f * 0.7f};

	mat_ambient[0] = r * 0.5;
	mat_ambient[1] = g * 0.5;
	mat_ambient[2] = b * 0.5;
	mat_ambient[3] = 0.0;

	mat_diffuse[0] = r;
	mat_diffuse[1] = g;
	mat_diffuse[2] = b;
	mat_diffuse[3] = 0.0;

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular );
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient );
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse );
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess );	
}


void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ī�޶� ��ġ ����
	SetupCamera();

	// ī�޶� �����
	SetupCameraProjection();

	// �𵨸� ��ȯ
	glMatrixMode(GL_MODELVIEW);

	// �ٴڱ׸���
	DrawFloor();	
	DrawObject(GL_RENDER);
			
	// ������۸��� ���� ��ȯ
	SwapBuffers(hDC);

	return;
}

void GetSphereCoord(int x, int y, float *px, float *py, float *pz)
{
	float r;
	*px = (2.0 * x - Width) / Width;
	*py = (-2.0 * y + Height) / Height;
	
	r = *px * *px + *py * *py;
	if (r >= 1.0)
	{
		*px = *px / sqrt(r);
		*py = *py / sqrt(r);
		*pz = 0.0;
	}
	else
	{
		*pz = sqrt(1.0 - r);
	}
}


void DrawFloor()
{
	glDisable(GL_LIGHTING);
	glColor3f(0.0f, 0.0f, 0.0f);

	int i, size = 100;
	for (i = -size; i <= size; i+=5)
	{
			if (i != 0)
			{
				glBegin(GL_LINES);
				glVertex3f((float)i, 0.0f, (float)size);
				glVertex3f((float)i, 0.0f, -(float)size);
				glVertex3f(-(float)size, 0.0f, (float)i);
				glVertex3f((float)size, 0.0f, (float)i);
				glEnd();
			}
			else
			{
				glBegin(GL_LINES);
				glColor3f(1.0f, 0.0f, 0.0f);
				glVertex3f(-(float)size, 0.0f, 0.0f);
				glVertex3f((float)size, 0.0f, 0.0f);
				glVertex3f(0.0f, 0.0f, (float)size);
				glVertex3f(0.0f, 0.0f, -(float)size);
				glEnd();
				glColor3f(0.0f, 0.0f, 0.0f);
			}
	}

	glEnable(GL_LIGHTING);
}




