#include "CollisionMath.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "00_Game/40_Collision/00_Box/BoxCollider.h"
#include "00_Game/40_Collision/00_Capsule/CapsuleCollider.h"
#include "00_Game/40_Collision/10_Sphere/SphereCollider.h"

namespace CollisionMath {

CollisionInfo TestCapsuleVsCapsule(const CapsuleCollider& A, const CapsuleCollider& B)
{
	using namespace DirectX;

	CollisionInfo info{};

	const XMVECTOR p1 = A.GetSegmentStart();
	const XMVECTOR p2 = A.GetSegmentEnd();
	const XMVECTOR q1 = B.GetSegmentStart();
	const XMVECTOR q2 = B.GetSegmentEnd();

	// 線分P(A側)と線分Q(B側)の最短点を求める(Closest Point Segment-Segment).
	const XMVECTOR r = XMVectorSubtract(p2, p1);
	const XMVECTOR s = XMVectorSubtract(q2, q1);
	const XMVECTOR e = XMVectorSubtract(p1, q1);

	const float a_val = XMVectorGetX(XMVector3Dot(r, r));
	const float e_val = XMVectorGetX(XMVector3Dot(r, s));
	const float f_val = XMVectorGetX(XMVector3Dot(s, s));
	const float g_val = XMVectorGetX(XMVector3Dot(r, e));
	const float h_val = XMVectorGetX(XMVector3Dot(s, e));

	constexpr float EPSILON = 1e-6f;
	float s_param = 0.0f;
	float t_param = 0.0f;
	const float k = a_val * f_val - e_val * e_val;

	if (k < EPSILON)
	{
		// 2線分がほぼ平行.
		s_param = 0.0f;
		t_param = (f_val < EPSILON) ? 0.0f : std::clamp(h_val / f_val, 0.0f, 1.0f);
	}
	else
	{
		const float inv_k = 1.0f / k;
		s_param = std::clamp((e_val * h_val - f_val * g_val) * inv_k, 0.0f, 1.0f);
		t_param = (f_val < EPSILON) ? 0.0f : std::clamp((s_param * e_val + h_val) / f_val, 0.0f, 1.0f);

		if (t_param == 0.0f || t_param == 1.0f)
		{
			s_param = (t_param == 0.0f) ? (-g_val / a_val) : ((e_val - g_val) / a_val);
			s_param = std::clamp(s_param, 0.0f, 1.0f);
		}
	}

	const XMVECTOR closest_p = XMVectorAdd(p1, XMVectorScale(r, s_param));
	const XMVECTOR closest_q = XMVectorAdd(q1, XMVectorScale(s, t_param));

	const XMVECTOR v_shortest = XMVectorSubtract(closest_q, closest_p); // A -> B方向.
	const float dist_sq = XMVectorGetX(XMVector3LengthSq(v_shortest));

	const float required_distance    = A.GetRadius() + B.GetRadius();
	const float required_distance_sq = required_distance * required_distance;

	if (dist_sq <= required_distance_sq)
	{
		info.IsHit = true;

		const float distance    = std::sqrtf(dist_sq);
		const XMVECTOR v_normal  = XMVector3Normalize(v_shortest);
		const XMVECTOR v_contact = XMVectorScale(XMVectorAdd(closest_p, closest_q), 0.5f);

		info.PenetrationDepth = required_distance - distance;
		XMStoreFloat3(&info.Normal, v_normal);
		XMStoreFloat3(&info.ContactPoint, v_contact);

		info.SelfCollider = &A;
		info.OtherCollider = &B;
	}

	return info;
}

CollisionInfo TestCapsuleVsSphere(const CapsuleCollider& Capsule, const SphereCollider& Sphere)
{
	using namespace DirectX;

	CollisionInfo info{};

	const XMVECTOR p1 = Capsule.GetSegmentStart();
	const XMVECTOR p2 = Capsule.GetSegmentEnd();

	const XMFLOAT3 sphere_position = Sphere.GetPosition();
	const XMVECTOR q = XMLoadFloat3(&sphere_position);

	// 点Q(球の中心)から線分P1P2(カプセルの中心線分)への最短点を求める.
	const XMVECTOR ab = XMVectorSubtract(p2, p1);
	const XMVECTOR ap = XMVectorSubtract(q, p1);

	const float e = XMVectorGetX(XMVector3Dot(ap, ab));
	const float f = XMVectorGetX(XMVector3LengthSq(ab));

	XMVECTOR closest_p;
	if (e <= 0.0f)      { closest_p = p1; }
	else if (e >= f)    { closest_p = p2; }
	else                { closest_p = XMVectorAdd(p1, XMVectorScale(ab, e / f)); }

	const XMVECTOR v_shortest = XMVectorSubtract(q, closest_p); // Capsule -> Sphere方向.
	const float dist_sq = XMVectorGetX(XMVector3LengthSq(v_shortest));

	const float required_distance    = Capsule.GetRadius() + Sphere.GetRadius();
	const float required_distance_sq = required_distance * required_distance;

	if (dist_sq <= required_distance_sq)
	{
		info.IsHit = true;

		const float distance   = std::sqrtf(dist_sq);
		const XMVECTOR v_normal = XMVector3Normalize(v_shortest);
		const XMVECTOR v_contact = XMVectorAdd(closest_p, XMVectorScale(v_normal, Capsule.GetRadius()));

		info.PenetrationDepth = required_distance - distance;
		XMStoreFloat3(&info.Normal, v_normal);
		XMStoreFloat3(&info.ContactPoint, v_contact);

		info.SelfCollider = &Capsule;
		info.OtherCollider = &Sphere;
	}

	return info;
}

CollisionInfo TestSphereVsSphere(const SphereCollider& A, const SphereCollider& B)
{
	using namespace DirectX;

	CollisionInfo info{};

	const XMFLOAT3 position_a = A.GetPosition();
	const XMFLOAT3 position_b = B.GetPosition();
	const XMVECTOR v_a = XMLoadFloat3(&position_a);
	const XMVECTOR v_b = XMLoadFloat3(&position_b);

	const XMVECTOR v_shortest = XMVectorSubtract(v_b, v_a); // A -> B方向.
	const float dist_sq = XMVectorGetX(XMVector3LengthSq(v_shortest));

	const float required_distance    = A.GetRadius() + B.GetRadius();
	const float required_distance_sq = required_distance * required_distance;

	if (dist_sq <= required_distance_sq)
	{
		info.IsHit = true;

		const float distance    = std::sqrtf(dist_sq);
		const XMVECTOR v_normal  = XMVector3Normalize(v_shortest);
		const XMVECTOR v_contact = XMVectorAdd(v_a, XMVectorScale(v_normal, A.GetRadius()));

		info.PenetrationDepth = required_distance - distance;
		XMStoreFloat3(&info.Normal, v_normal);
		XMStoreFloat3(&info.ContactPoint, v_contact);

		info.SelfCollider = &A;
		info.OtherCollider = &B;
	}

	return info;
}

CollisionInfo TestBoxVsBox(const BoxCollider& A, const BoxCollider& B)
{
	using namespace DirectX;

	CollisionInfo info{};

	const XMFLOAT3 pos_a = A.GetPosition();
	const XMFLOAT3 pos_b = B.GetPosition();
	const XMVECTOR center_a = XMLoadFloat3(&pos_a);
	const XMVECTOR center_b = XMLoadFloat3(&pos_b);
	const XMVECTOR to_b     = XMVectorSubtract(center_b, center_a); // A -> B方向.

	const XMFLOAT3& size_a = A.GetSize();
	const XMFLOAT3& size_b = B.GetSize();
	const float half_ax = size_a.x * 0.5f, half_ay = size_a.y * 0.5f, half_az = size_a.z * 0.5f;
	const float half_bx = size_b.x * 0.5f, half_by = size_b.y * 0.5f, half_bz = size_b.z * 0.5f;

	const XMVECTOR axis_ax = A.GetLocalAxisX();
	const XMVECTOR axis_az = A.GetLocalAxisZ();
	const XMVECTOR axis_bx = B.GetLocalAxisX();
	const XMVECTOR axis_bz = B.GetLocalAxisZ();
	const XMVECTOR axis_y  = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // Yaw回転のみのため両者共通.

	// 分離軸判定(SAT). BoxはYawのみで回転するため、必要な分離軸はY軸とお互いのX/Z軸の
	// 5本で十分(一般的なOBB-OBBは最大15本の軸が必要だが、水平回転のみという制約のため
	// 残りの軸は冗長になる).
	const XMVECTOR candidate_axes[5] = { axis_y, axis_ax, axis_az, axis_bx, axis_bz };

	float min_overlap = std::numeric_limits<float>::max();
	XMVECTOR best_normal = axis_y;

	for (const XMVECTOR& axis : candidate_axes)
	{
		const float radius_a = std::fabs(XMVectorGetX(XMVector3Dot(axis_ax, axis))) * half_ax
		                      + std::fabs(XMVectorGetY(axis)) * half_ay
		                      + std::fabs(XMVectorGetX(XMVector3Dot(axis_az, axis))) * half_az;
		const float radius_b = std::fabs(XMVectorGetX(XMVector3Dot(axis_bx, axis))) * half_bx
		                      + std::fabs(XMVectorGetY(axis)) * half_by
		                      + std::fabs(XMVectorGetX(XMVector3Dot(axis_bz, axis))) * half_bz;

		const float signed_distance = XMVectorGetX(XMVector3Dot(to_b, axis));
		const float overlap = (radius_a + radius_b) - std::fabs(signed_distance);

		if (overlap < 0.0f) { return info; } // この軸で分離している = 衝突していない.

		if (overlap < min_overlap)
		{
			min_overlap  = overlap;
			best_normal  = (signed_distance >= 0.0f) ? axis : XMVectorNegate(axis);
		}
	}

	info.IsHit = true;
	info.PenetrationDepth = min_overlap;
	XMStoreFloat3(&info.Normal, XMVector3Normalize(best_normal));

	// 接触点は近似(両中心の中点. 厳密な接触点の算出は行わない).
	const XMVECTOR v_contact = XMVectorScale(XMVectorAdd(center_a, center_b), 0.5f);
	XMStoreFloat3(&info.ContactPoint, v_contact);

	info.SelfCollider = &A;
	info.OtherCollider = &B;

	return info;
}

CollisionInfo TestBoxVsSphere(const BoxCollider& Box, const SphereCollider& Sphere)
{
	using namespace DirectX;

	CollisionInfo info{};

	const XMFLOAT3 box_pos = Box.GetPosition();
	const XMVECTOR v_box_center = XMLoadFloat3(&box_pos);

	const XMFLOAT3 sphere_pos = Sphere.GetPosition();
	const XMVECTOR v_sphere_center = XMLoadFloat3(&sphere_pos);

	const XMVECTOR axis_x = Box.GetLocalAxisX();
	const XMVECTOR axis_z = Box.GetLocalAxisZ();
	const XMFLOAT3& size  = Box.GetSize();

	const XMVECTOR to_sphere = XMVectorSubtract(v_sphere_center, v_box_center);

	// ローカル軸への射影(=ローカル座標系での球中心座標)をhalfExtentでクランプし、
	// Box表面上の最近接点を求める(Y軸はYaw回転で不変なのでワールドYをそのまま使う).
	const float local_x = std::clamp(XMVectorGetX(XMVector3Dot(to_sphere, axis_x)), -size.x * 0.5f, size.x * 0.5f);
	const float local_y = std::clamp(XMVectorGetY(to_sphere), -size.y * 0.5f, size.y * 0.5f);
	const float local_z = std::clamp(XMVectorGetX(XMVector3Dot(to_sphere, axis_z)), -size.z * 0.5f, size.z * 0.5f);

	XMVECTOR closest = XMVectorAdd(v_box_center, XMVectorScale(axis_x, local_x));
	closest = XMVectorAdd(closest, XMVectorScale(axis_z, local_z));
	closest = XMVectorAdd(closest, XMVectorSet(0.0f, local_y, 0.0f, 0.0f));

	const XMVECTOR v_shortest = XMVectorSubtract(v_sphere_center, closest); // Box -> Sphere方向.
	const float dist_sq = XMVectorGetX(XMVector3LengthSq(v_shortest));
	const float radius  = Sphere.GetRadius();

	if (dist_sq <= radius * radius)
	{
		info.IsHit = true;

		const float distance = std::sqrtf(dist_sq);
		// 球の中心がBox内部にある場合(distance≈0)は法線が不定になるため上向きにフォールバック.
		const XMVECTOR v_normal = (dist_sq > 1e-8f) ? XMVector3Normalize(v_shortest) : XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		info.PenetrationDepth = radius - distance;
		XMStoreFloat3(&info.Normal, v_normal);
		XMStoreFloat3(&info.ContactPoint, closest);

		info.SelfCollider = &Box;
		info.OtherCollider = &Sphere;
	}

	return info;
}

CollisionInfo TestBoxVsCapsule(const BoxCollider& Box, const CapsuleCollider& Capsule)
{
	using namespace DirectX;

	CollisionInfo info{};

	// CapsuleColliderの中心線分は構造上常にワールドY軸に平行(GetSegmentStart/Endが
	// 持ち主のYawでしか回転しないため、鉛直方向のオフセットは常に鉛直のまま).
	// これを利用し、水平面(XZ)は「半径Radiusの円 vs Box」、高さ(Y)は区間の重なりとして
	// 分けて判定する(一般的な線分 vs OBBのSATを実装するより単純かつこの用途では正確).
	const XMVECTOR p1 = Capsule.GetSegmentStart();
	const XMVECTOR p2 = Capsule.GetSegmentEnd();
	const float radius = Capsule.GetRadius();

	const XMFLOAT3 box_pos = Box.GetPosition();
	const XMVECTOR v_box_center = XMLoadFloat3(&box_pos);

	const XMVECTOR axis_x = Box.GetLocalAxisX();
	const XMVECTOR axis_z = Box.GetLocalAxisZ();
	const XMFLOAT3& size  = Box.GetSize();
	const float half_y = size.y * 0.5f;

	// 高さ(Y)方向の区間オーバーラップ判定.
	const float box_min_y     = XMVectorGetY(v_box_center) - half_y;
	const float box_max_y     = XMVectorGetY(v_box_center) + half_y;
	const float capsule_min_y = std::min(XMVectorGetY(p1), XMVectorGetY(p2));
	const float capsule_max_y = std::max(XMVectorGetY(p1), XMVectorGetY(p2));

	if (std::min(box_max_y, capsule_max_y) - std::max(box_min_y, capsule_min_y) < 0.0f)
	{
		return info; // 高さで分離.
	}

	// 水平面(XZ): カプセル中心線(P1のXZ. 鉛直なのでP2でも同じ)からBoxのローカル軸へ
	// クランプした最近接点との距離で判定する(Box-vs-Sphereと同様の手法).
	const XMVECTOR to_capsule = XMVectorSubtract(p1, v_box_center);
	const float local_x = std::clamp(XMVectorGetX(XMVector3Dot(to_capsule, axis_x)), -size.x * 0.5f, size.x * 0.5f);
	const float local_z = std::clamp(XMVectorGetX(XMVector3Dot(to_capsule, axis_z)), -size.z * 0.5f, size.z * 0.5f);
	const float clamped_y = std::clamp(XMVectorGetY(v_box_center), capsule_min_y, capsule_max_y);

	XMVECTOR closest_on_box = XMVectorAdd(v_box_center, XMVectorScale(axis_x, local_x));
	closest_on_box = XMVectorAdd(closest_on_box, XMVectorScale(axis_z, local_z));
	closest_on_box = XMVectorSetY(closest_on_box, clamped_y);

	const XMVECTOR closest_on_capsule = XMVectorSetY(p1, clamped_y);

	const XMVECTOR v_shortest = XMVectorSubtract(closest_on_capsule, closest_on_box); // Box -> Capsule方向.
	const float dist_sq = XMVectorGetX(XMVector3LengthSq(v_shortest));

	if (dist_sq <= radius * radius)
	{
		info.IsHit = true;

		const float distance = std::sqrtf(dist_sq);
		const XMVECTOR v_normal = (dist_sq > 1e-8f) ? XMVector3Normalize(v_shortest) : XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		info.PenetrationDepth = radius - distance;
		XMStoreFloat3(&info.Normal, v_normal);
		XMStoreFloat3(&info.ContactPoint, closest_on_box);

		info.SelfCollider = &Box;
		info.OtherCollider = &Capsule;
	}

	return info;
}

} // namespace CollisionMath
