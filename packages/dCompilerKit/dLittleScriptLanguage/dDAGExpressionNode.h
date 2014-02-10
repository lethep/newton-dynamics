/* Copyright (c) <2009> <Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#ifndef __dDAGExpressionNode_H__
#define __dDAGExpressionNode_H__

#include "dDAG.h"
#include "dLSCstdafx.h"
class dDAGExpressionNodeVariable;


class dDAGExpressionNode: public dDAG
{
	public:
	class dDAGEvaluation
	{
		public:
		enum dDAGType
		{
			m_int,
			m_float
		};

		dDAGEvaluation ()
			:m_type(m_int)
		{
			m_f = 0.0;
		}

		dDAGType m_type;
		union {;
			dMachineIntRegister m_i;
			dMachineFloatRegister m_f;
		};
	};

	dDAGExpressionNode (dList<dDAG*>& allNodes);
	~dDAGExpressionNode(void);

	virtual dDAGEvaluation Evalue(dCIL& cil) {dAssert (0); return dDAGEvaluation();}
	virtual void CompileCIL(dCIL& cil) {dAssert (0);}
	virtual void ConnectParent(dDAG* const parent)  {dAssert (0);}
	virtual dDAGExpressionNodeVariable* FindLeftVariable() {dAssert (0); return NULL; }
	dDAGRtti(dDAG);
};


#endif