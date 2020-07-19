//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#include "Camera.h"
#include "Math.h"

#define CAMERA_DEBUG 1

using namespace DirectX;

Camera::Camera()
	:
	MoveSpeed(1000.0f),
	AngularSpeedDeg(20.0f),
	Drag(9.5f),
	mPitch(0.0f),
	mYaw(0.0f),
	mPosition(0,0,0),
	mVelocity(0,0,0)
{
	XMStoreFloat4x4(&mMatProj, XMMatrixIdentity());
	XMStoreFloat4x4(&mMatView, XMMatrixIdentity());
}

Camera::~Camera(void)
{}

void Camera::InitializeCamera(const FCameraData& data)
{
	const auto& NEAR_PLANE = data.nearPlane;
	const auto& FAR_PLANE = data.farPlane;
	const float& AspectRatio = data.aspect;
	const float VerticalFoV = data.fovV_Degrees * DEG2RAD;

	//SetOthoMatrix(ViewportX, ViewportY, NEAR_PLANE, FAR_PLANE); // quick test
	SetProjectionMatrix(VerticalFoV, AspectRatio, NEAR_PLANE, FAR_PLANE);

	SetPosition(data.x, data.y, data.z);
	mYaw = mPitch = 0;
	Rotate(data.yaw * DEG2RAD, data.pitch * DEG2RAD, 1.0f);
}


void Camera::SetOthoMatrix(int screenWidth, int screenHeight, float screenNear, float screenFar)
{
	XMStoreFloat4x4(&mMatProj, XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenFar));
}

void Camera::SetProjectionMatrix(float fovy, float screenAspect, float screenNear, float screenFar)
{
	XMStoreFloat4x4(&mMatProj, XMMatrixPerspectiveFovLH(fovy, screenAspect, screenNear, screenFar));
}

void Camera::SetProjectionMatrixHFov(float fovx, float screenAspectInverse, float screenNear, float screenFar)
{	// horizonital FOV
	const float FarZ = screenFar; float NearZ = screenNear;
	const float r = screenAspectInverse;
	
	const float Width = 1.0f / tanf(fovx*0.5f);
	const float Height = Width / r;
	const float fRange = FarZ / (FarZ - NearZ);

	XMMATRIX M;	
	M.r[0].m128_f32[0] = Width;
	M.r[0].m128_f32[1] = 0.0f;
	M.r[0].m128_f32[2] = 0.0f;
	M.r[0].m128_f32[3] = 0.0f;

	M.r[1].m128_f32[0] = 0.0f;
	M.r[1].m128_f32[1] = Height;
	M.r[1].m128_f32[2] = 0.0f;
	M.r[1].m128_f32[3] = 0.0f;

	M.r[2].m128_f32[0] = 0.0f;
	M.r[2].m128_f32[1] = 0.0f;
	M.r[2].m128_f32[2] = fRange;
	M.r[2].m128_f32[3] = 1.0f;

	M.r[3].m128_f32[0] = 0.0f;
	M.r[3].m128_f32[1] = 0.0f;
	M.r[3].m128_f32[2] = -fRange * NearZ;
	M.r[3].m128_f32[3] = 0.0f;
	XMStoreFloat4x4(&mMatProj, M);
}


void Camera::Update(const float dt, const FCameraInput& input)
{
	Rotate(dt, input);
	Move(dt, input);

	XMVECTOR up         = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAt     = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const XMVECTOR pos  = XMLoadFloat3(&mPosition);
	const XMMATRIX MRot = GetRotationMatrix();

	//transform the lookat and up vector by rotation matrix
	lookAt	= XMVector3TransformCoord(lookAt, MRot);
	up		= XMVector3TransformCoord(up,	  MRot);

	//translate the lookat
	lookAt = pos + lookAt;

	//create view matrix
	XMStoreFloat4x4(&mMatView, XMMatrixLookAtLH(pos, lookAt, up));

	// move based on velocity
	XMVECTOR P = XMLoadFloat3(&mPosition);
	XMVECTOR V = XMLoadFloat3(&mVelocity);
	P += V * dt;
	XMStoreFloat3(&mPosition, P);
}


XMFLOAT3 Camera::GetPositionF() const
{
	return mPosition;
}

XMMATRIX Camera::GetViewMatrix() const
{
	return XMLoadFloat4x4(&mMatView);
}

XMMATRIX Camera::GetViewInverseMatrix() const
{
	const XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	const XMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	const XMVECTOR pos    = XMLoadFloat3(&mPosition);
	const XMMATRIX MRot   = GetRotationMatrix();

	const XMVECTOR dir    = XMVector3Normalize(lookAt - pos);
	const XMVECTOR wing   = XMVector3Cross(up, dir);

	XMMATRIX R = XMMatrixIdentity(); 
	R.r[0] = wing;
	R.r[1] = up;
	R.r[2] = dir;
	R.r[0].m128_f32[3] = R.r[1].m128_f32[3] = R.r[2].m128_f32[3] = 0;
	//R = XMMatrixTranspose(R);
	
	XMMATRIX T = XMMatrixIdentity();
	T.r[3] = pos;
	T.r[3].m128_f32[3] = 1.0;
	
	XMMATRIX viewInverse = R * T;	// this is for ViewMatrix
	//	orienting our model using this, we want the inverse of view mat
	// XMMATRIX rotMatrix = XMMatrixTranspose(R) * T.inverse();

	XMMATRIX view = XMLoadFloat4x4(&mMatView);
	XMVECTOR det = XMMatrixDeterminant(view);
	XMMATRIX test = XMMatrixInverse(&det, view);

	return test;
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return  XMLoadFloat4x4(&mMatProj);
}

FFrustumPlaneset Camera::GetViewFrustumPlanes() const
{
	return FFrustumPlaneset::ExtractFromMatrix(GetViewMatrix() * GetProjectionMatrix());
}

XMMATRIX Camera::GetRotationMatrix() const
{
	return XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);
}

void Camera::SetPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
}

void Camera::Rotate(float yaw, float pitch, const float dt)
{
	mYaw   += yaw   * dt;
	mPitch += pitch * dt;
	
	if (mPitch > +90.0f * DEG2RAD) mPitch = +90.0f * DEG2RAD;
	if (mPitch < -90.0f * DEG2RAD) mPitch = -90.0f * DEG2RAD;
}


//void Camera::Reset() // TODO: input
//{
//	const Settings::Camera & data = m_settings;
//	SetPosition(data.x, data.y, data.z);
//	mYaw = mPitch = 0;
//	Rotate(data.yaw * DEG2RAD, data.pitch * DEG2RAD, 1.0f);
//}

// internal update functions
void Camera::Rotate(const float dt, const FCameraInput& input)
{
	float dy = input.DeltaMouseXY[1];
	float dx = input.DeltaMouseXY[0];

	const float delta = AngularSpeedDeg * DEG2RAD * dt;
	Rotate(dx, dy, delta);
}

void Camera::Move(const float dt, const FCameraInput& input)
{
	const XMMATRIX MRotation	 = GetRotationMatrix();
	const XMVECTOR WorldSpaceTranslation = XMVector3TransformCoord(input.LocalTranslationVector, MRotation);

	XMVECTOR V = XMLoadFloat3(&mVelocity);
	V += (WorldSpaceTranslation * MoveSpeed - V * Drag) * dt;
	XMStoreFloat3(&mVelocity, V);
}
