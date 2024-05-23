#include <windows.h>
#include <gl/glut.h>
#include <vector>
#include <cmath>
#include<winuser.h> 

using namespace std;

#ifndef WM_MOUSEWHEEL 
#define WM_MOUSEWHEEL 0x020A 
#endif 

// 전역변수
HDC hDC;
HGLRC hRC;
int Width, Height;

// 관측변환을 위한 변수
int StartPt[2];

// ROTATE를 위한 변수
float Axis[3] = {1.0, 0.0, 0.0};
float Angle = 0.0;
float RotMat[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

// PAN 을 위한 변수
float dT[3] = {0.0, 0.0, 0.0};

// ZOOM을 위한 변수
float ViewSize = 50.0;

float backWheelAngle = 0.0, armAngle = -20.0, frontWheelAngle = 0.0, shovelAngle = 10.0;

#define BUFFER_SIZE 64
GLuint SelectBuffer[BUFFER_SIZE];
#define BACKWHEELANGLE 10
#define ARMANGLE 11
#define FRONTWHEELANGLE 12
#define SHOVELANGLE 12

// 함수선언
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool SetupPixelFormat();
void DrawScene();
void SetupCamera();
void SetupViewport();
void SetupCameraProjection();

// 사용자 정의 함수
void InitOpenGL();
void GetSphereCoord(int x, int y, float *px, float *py, float *pz);
void DrawFloor();
void SetMaterial(float r, float g, float b);
void Picking(int x, int y);
void DrawObject(GLenum mode);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// 윈도우 클래스 등록
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

	// 윈도우 생성
	HWND hWnd = CreateWindow("OpenGL Window Class", "3D SceneViewer", 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1008, 1027, 0, 0, hInstance, 0);

	// 윈도우 출력
	ShowWindow(hWnd, nShowCmd);
	UpdateWindow(hWnd);

	// 메시지 루프진입
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
		// 디바이스컨텍스트를 얻음
		hDC = GetDC(hWnd);		
		if (!SetupPixelFormat()) 
			DestroyWindow(hWnd); 

		// 렌더링컨텍스트를 얻음
		hRC = wglCreateContext(hDC); 
		wglMakeCurrent(hDC, hRC);

		// OpenGL 옵션 초기화
		InitOpenGL();
		break; 

	case WM_SIZE: 
		GetClientRect(hWnd, &rect);
		Width = rect.right;
		Height = rect.bottom;

		// 뷰포트 변환
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
		// 장면그리기
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
		// 테스트 객체 그리기(불도저)

	// 기본 색상(노랑)
		SetMaterial(1.0, 1.0, 0.0);
		// 트럭 트렁크
		glPushMatrix();
			glTranslatef(-5.0, 12.0, 0.0);
			glScalef(15.0, 14.0, 15.0);
			glutSolidCube(1.0);
		glPopMatrix();

		// 트럭 머리
		glPushMatrix();
			// 외벽
			glTranslatef(6.0, 11.0, 0.0);
			glScalef(8.0, 12.0, 15.0);
			glutSolidCube(1.0);

			// 유리(하늘색)
			glTranslatef(0.6, 0.1, 0.0);
			glScalef(0.3, 0.75, 0.9);
			SetMaterial(0.0, 3.5, 3.5);
			glutSolidCube(1.0);

			glTranslatef(0.0, -0.6, 0.0);
			glScalef(1.5, 0.4, 1.1);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
		glPopMatrix();

		//바퀴(짙은 회색)
		// angle2에 관한 회전
		glPushMatrix();
		SetMaterial(0.1, 0.1, 0.1);
		// 앞 바퀴
		glPushMatrix();
		glTranslatef(7.0, 5.0, 0);
		glScalef(1.0, 1.0, 1.0);
		// 바퀴 우
		glPushMatrix();
		glTranslatef(0.0, 0.0, 7.5);
		glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// 휠
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(1.0);
		glPopMatrix();

		// 바퀴 좌
		glPushMatrix();
		glTranslatef(0.0, 0.0, -7.5);
		glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// 휠
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(1.0);
		glPopMatrix();
		glPopMatrix();
		glPopMatrix();

		// angle0에 관한 회전
		glPushMatrix();
		SetMaterial(0.1, 0.1, 0.1);
		glTranslatef(-7.0, 5.0, 0.0);
		glRotatef(backWheelAngle, 0.0, 0.0, 1.0);
		// 뒷 바퀴 우(관절)
		glPushMatrix();
		glTranslatef(0.0, 0.0, 7.5);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// 휠(인데 회전하는 것을 확인하기 위해 우측 만 크기 축소)
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(0.3);
		glPopMatrix();

		// 뒷 바퀴 좌(관절)
		glPushMatrix();
		glTranslatef(0.0, 0.0, -7.5);
		glutSolidTorus(1.0, 2.0, 100, 100);
		// 휠
		glScalef(3.0, 3.0, 1.0);
		glutSolidCube(1.0);
		glPopMatrix();
		glPopMatrix();

		// angle1에 관한 회전
		glPushMatrix();
		glTranslatef(5.0, 10.0, 0.0);
		glRotatef(armAngle, 0.0, 0.0, 1.0);
		// 팔 우(관절)
		glPushMatrix();
		glTranslatef(0.0, 0.0, 8.5);
		glScalef(2.0, 2.0, 2.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		// 팔뚝 추가
		glTranslatef(4.0, 0.0, 0.0);
		glScalef(7.5, 1.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();

		// 팔 좌(관절)
		glPushMatrix();
		glTranslatef(0.0, 0.0, -8.5);
		glScalef(2.0, 2.0, 2.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		// 팔뚝 추가
		glTranslatef(4.0, 0.0, 0.0);
		glScalef(7.5, 1.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();


		// angle3에 관한 회전
		glPushMatrix();
		glTranslatef(15.0, 0.0, 0.0);
		glRotatef(shovelAngle, 0.0, 0.0, 1.0);
		// 삽(관절)
		glScalef(1.5, 1.2, 1.0);
		glPushMatrix();
		glTranslatef(0.0, 0.0, 0.0);
		glScalef(1.0, 1.0, 15.0);
		SetMaterial(0.1, 0.1, 0.1);
		glutSolidCube(1.0);
		// 삽 (짙은 회색)
		// 가로
		glPushMatrix();
		glTranslatef(4.0, 0.0, 0.0);
		glScalef(7.0, 1.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();
		// 세로
		glPushMatrix();
		glTranslatef(0.0, 3.5, 0.0);
		glScalef(1.0, 6.0, 1.0);
		SetMaterial(1.0, 1.0, 0.0);
		glutSolidCube(1.0);
		glPopMatrix();

		// 외벽(내외 확인을 위해 우측 벽은 생략)
		glScalef(1.0, 1.0, 1.0 / 15.0); // 사이즈 복구
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
		// 테스트 객체 그리기(불도저)

			// 기본 색상(노랑)
			SetMaterial(1.0, 1.0, 0.0);
			// 트럭 트렁크
			glPushMatrix();
			glTranslatef(-5.0, 12.0, 0.0);
			glScalef(15.0, 14.0, 15.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// 트럭 머리
			glPushMatrix();
			// 외벽
			glTranslatef(6.0, 11.0, 0.0);
			glScalef(8.0, 12.0, 15.0);
			glutSolidCube(1.0);

			// 유리(하늘색)
			glTranslatef(0.6, 0.1, 0.0);
			glScalef(0.3, 0.75, 0.9);
			SetMaterial(0.0, 3.5, 3.5);
			glutSolidCube(1.0);

			glTranslatef(0.0, -0.6, 0.0);
			glScalef(1.5, 0.4, 1.1);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();

			//바퀴(짙은 회색)
			// angle2에 관한 회전
			glPushMatrix();
			SetMaterial(0.1, 0.1, 0.1);
			// 앞 바퀴
			glPushMatrix();
			glTranslatef(7.0, 5.0, 0);
			glScalef(1.0, 1.0, 1.0);
			// 바퀴 우
			glPushMatrix();
			glTranslatef(0.0, 0.0, 7.5);
			glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// 휠
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// 바퀴 좌
			glPushMatrix();
			glTranslatef(0.0, 0.0, -7.5);
			glRotatef(frontWheelAngle, 0.0, 1.0, 0.0);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// 휠
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(1.0);
			glPopMatrix();
			glPopMatrix();
			glPopMatrix();

			// angle0에 관한 회전
			glPushMatrix();
			SetMaterial(0.1, 0.1, 0.1);
			glTranslatef(-7.0, 5.0, 0.0);
			glRotatef(backWheelAngle, 0.0, 0.0, 1.0);
			// 뒷 바퀴 우(관절)
			glPushMatrix();
			glTranslatef(0.0, 0.0, 7.5);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// 휠(인데 회전하는 것을 확인하기 위해 우측 만 크기 축소)
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(0.3);
			glPopMatrix();

			// 뒷 바퀴 좌(관절)
			glPushMatrix();
			glTranslatef(0.0, 0.0, -7.5);
			glutSolidTorus(1.0, 2.0, 100, 100);
			// 휠
			glScalef(3.0, 3.0, 1.0);
			glutSolidCube(1.0);
			glPopMatrix();
			glPopMatrix();

			// angle1에 관한 회전
			glPushMatrix();
			glTranslatef(5.0, 10.0, 0.0);
			glRotatef(armAngle, 0.0, 0.0, 1.0);
			// 팔 우(관절)
			glPushMatrix();
			glTranslatef(0.0, 0.0, 8.5);
			glScalef(2.0, 2.0, 2.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			// 팔뚝 추가
			glTranslatef(4.0, 0.0, 0.0);
			glScalef(7.5, 1.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// 팔 좌(관절)
			glPushMatrix();
			glTranslatef(0.0, 0.0, -8.5);
			glScalef(2.0, 2.0, 2.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			// 팔뚝 추가
			glTranslatef(4.0, 0.0, 0.0);
			glScalef(7.5, 1.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();


			// angle3에 관한 회전
			glPushMatrix();
			glTranslatef(15.0, 0.0, 0.0);
			glRotatef(shovelAngle, 0.0, 0.0, 1.0);
			// 삽(관절)
			glScalef(1.5, 1.2, 1.0);
			glPushMatrix();
			glTranslatef(0.0, 0.0, 0.0);
			glScalef(1.0, 1.0, 15.0);
			SetMaterial(0.1, 0.1, 0.1);
			glutSolidCube(1.0);
			// 삽 (짙은 회색)
			// 가로
			glPushMatrix();
			glTranslatef(4.0, 0.0, 0.0);
			glScalef(7.0, 1.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();
			// 세로
			glPushMatrix();
			glTranslatef(0.0, 3.5, 0.0);
			glScalef(1.0, 6.0, 1.0);
			SetMaterial(1.0, 1.0, 0.0);
			glutSolidCube(1.0);
			glPopMatrix();

			// 외벽(내외 확인을 위해 우측 벽은 생략)
			glScalef(1.0, 1.0, 1.0 / 15.0); // 사이즈 복구
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

	// 1. 선택버퍼 지정
	memset(SelectBuffer, 0, BUFFER_SIZE);
	glSelectBuffer(BUFFER_SIZE, SelectBuffer);

	// 2. 렌더링 모드 변경
	glRenderMode(GL_SELECT);

	// 3. 네임스택 초기화 및 0입력
	glInitNames();
	glPushName(0);

	// 4. 투영행렬 선택 및 이전 투영행렬 저장 후, 단위행렬 설정
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// 5. 뷰포트 변환 정보를 얻어, 선택을 위한 투영행렬 생성 및 현재 행렬(단위 행렬)에 곱하기
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluPickMatrix((double)x, (double)(viewport[3] - y), 2.0, 2.0, viewport);

	// 6. 투영변환 행렬 생성 및 현재 행렬에 곱하기
	glOrtho(-ViewSize, ViewSize, -ViewSize, ViewSize, -1000.0f, 1000.0f);


	// 7. 아이디를 네임스택에 저장하면서 객체그리기
	DrawObject(GL_SELECT);

	// 8. 투영 행렬 선택 및 이전 투영행렬 복원
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	// 9. 렌더링 모드 변경, 선택된 객체의 개수 반환
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
	// OpenGL 초기화
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

	// 카메라 위치 뒤로 빼기
	glTranslatef(0.0f, 0.0f, -10.0f);
	glTranslatef(dT[0], dT[1], dT[2]);

	// 월드좌표계 회전
	glRotatef(Angle, Axis[0], Axis[1], Axis[2]);
	glMultMatrixf(RotMat);
	glGetFloatv(GL_MODELVIEW_MATRIX, RotMat);
	RotMat[12] = RotMat[13] = RotMat[14] = 0.0;
}

void SetupCameraProjection()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// 직교투영 행렬설정
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

	// 카메라 위치 설정
	SetupCamera();

	// 카메라 렌즈설정
	SetupCameraProjection();

	// 모델링 변환
	glMatrixMode(GL_MODELVIEW);

	// 바닥그리기
	DrawFloor();	
	DrawObject(GL_RENDER);
			
	// 더블버퍼링을 위한 교환
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




