/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


#ifndef __D_VEHICLE_SOLVER_H__
#define __D_VEHICLE_SOLVER_H__

#include "dStdafxVehicle.h"

class dVehicleNode;
class dVehicleChassis;

class dVehicleSolver: public dContainersAlloc
{
	class dBodyJointMatrixDataPair;

	public:
	DVEHICLE_API dVehicleSolver();
	DVEHICLE_API virtual ~dVehicleSolver();
	DVEHICLE_API void Finalize(dVehicleChassis* const vehicle);

	void Update(dFloat timestep);

	private:
	int CalculateNodeCount () const;
	void SortGraph(dVehicleNode* const root, int& index);

	void InitMassMatrix();
	int Factorize(dVehicleNode* const node);
	void GetJacobians(dVehicleNode* const node);
	int BuildJacobianMatrix(dFloat timestep);

	void CalculateJointDiagonal(dVehicleNode* const node);
	void CalculateJacobianBlock(dVehicleNode* const node);
	void CalculateBodyDiagonal(dVehicleNode* const child);
	void CalculateInertiaMatrix(dVehicleNode* const node) const;

	dVehicleChassis* m_vehicle;
	dVehicleNode** m_nodesOrder;

	// cache temporary variables
	//dSpatialMatrix* m_bodyJt;
	//dSpatialMatrix* m_jointJ;
	//dSpatialMatrix* m_bodyInvMass;
	//dSpatialMatrix* m_jointInvMass;
	//dSpatialMatrix* m_bodyMassArray;
	//dSpatialMatrix* m_jointMassArray;

	dBodyJointMatrixDataPair* m_data;
	
	dComplementaritySolver::dJacobianPair* m_leftHandSide;
	dComplementaritySolver::dJacobianColum* m_rightHandSide;
	
	int m_nodeCount;
	int m_rowsCount;
};


#endif 

