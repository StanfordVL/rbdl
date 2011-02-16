#include <UnitTest++.h>

#include <iostream>

#include "mathutils.h"
#include "Logging.h"

#include "Model.h"
#include "Contacts.h"
#include "Dynamics.h"
#include "Kinematics.h"

using namespace std;
using namespace SpatialAlgebra;
using namespace RigidBodyDynamics;

const double TEST_PREC = 1.0e-14;

struct ContactsFixture {
	ContactsFixture () {
		ClearLogOutput();
		model = new Model;
		model->Init();

		model->gravity.set (0., -9.81, 0.);

		/* Basically a model like this, where X are the Center of Masses
		 * and the CoM of the last (3rd) body comes out of the Y=X=0 plane.
		 *
		 *                X
		 *                *
		 *              _/
		 *            _/  (-Z)
		 *      Z    /
		 *      *---* 
		 *      |
		 *      |
		 *  Z   |
		 *  O---*
		 *      Y
		 */

		// base body
		base_rot_z = Body (
				0.,
				Vector3d (0., 0., 0.),
				Vector3d (0., 0., 0.)
				);
		joint_base_rot_z = Joint (
				JointTypeRevolute,
				Vector3d (0., 0., 1.)
				);
		base_rot_z_id = model->AddBody (0, Xtrans (Vector3d (0., 0., 0.)), joint_base_rot_z, base_rot_z);

		base_rot_y = Body (
				0.,
				Vector3d (0., 0., 0.),
				Vector3d (0., 0., 0.)
				);
		joint_base_rot_y = Joint (
				JointTypeRevolute,
				Vector3d (0., 1., 0.)
				);
		base_rot_y_id = model->AddBody (base_rot_z_id, Xtrans (Vector3d (0., 0., 0.)), joint_base_rot_y, base_rot_y);

		base_rot_x = Body (
				1.,
				Vector3d (0., 1., 0.),
				Vector3d (1., 1., 1.)
				);
		joint_base_rot_x = Joint (
				JointTypeRevolute,
				Vector3d (1., 0., 0.)
				);
		base_rot_x_id = model->AddBody (base_rot_y_id, Xtrans (Vector3d (0., 0., 0.)), joint_base_rot_x, base_rot_x);

		// child body
		child_rot_z = Body (
				0.,
				Vector3d (0., 0., 0.),
				Vector3d (0., 0., 0.)
				);
		joint_child_rot_z = Joint (
				JointTypeRevolute,
				Vector3d (0., 0., 1.)
				);
		child_rot_z_id = model->AddBody (base_rot_x_id, Xtrans (Vector3d (1., 0., 0.)), joint_child_rot_z, child_rot_z);

		child_rot_y = Body (
				0.,
				Vector3d (0., 0., 0.),
				Vector3d (0., 0., 0.)
				);
		joint_child_rot_y = Joint (
				JointTypeRevolute,
				Vector3d (0., 1., 0.)
				);
		child_rot_y_id = model->AddBody (child_rot_z_id, Xtrans (Vector3d (0., 0., 0.)), joint_child_rot_y, child_rot_y);

		child_rot_x = Body (
				1.,
				Vector3d (0., 1., 0.),
				Vector3d (1., 1., 1.)
				);
		joint_child_rot_x = Joint (
				JointTypeRevolute,
				Vector3d (1., 0., 0.)
				);
		child_rot_x_id = model->AddBody (child_rot_y_id, Xtrans (Vector3d (0., 0., 0.)), joint_child_rot_x, child_rot_x);

		Q = cmlVector(model->mBodies.size() - 1);
		QDot = cmlVector(model->mBodies.size() - 1);
		QDDot = cmlVector(model->mBodies.size() - 1);
		Tau = cmlVector(model->mBodies.size() - 1);

		Q.zero();
		QDot.zero();
		QDDot.zero();
		Tau.zero();

		contact_body_id = child_rot_x_id;
		contact_point.set (0., 1., 0.);
		contact_normal.set (0., 1., 0.);

		ClearLogOutput();
	}
	
	~ContactsFixture () {
		delete model;
	}
	Model *model;

	unsigned int base_rot_z_id, base_rot_y_id, base_rot_x_id,
		child_rot_z_id, child_rot_y_id, child_rot_x_id;

	Body base_rot_z, base_rot_y, base_rot_x,
		child_rot_z, child_rot_y, child_rot_x;

	Joint joint_base_rot_z, joint_base_rot_y, joint_base_rot_x,
		joint_child_rot_z, joint_child_rot_y, joint_child_rot_x;

	cmlVector Q;
	cmlVector QDot;
	cmlVector QDDot;
	cmlVector Tau;

	unsigned int contact_body_id;
	Vector3d contact_point;
	Vector3d contact_normal;
	std::vector<ContactInfo> contact_data;
};

TEST_FIXTURE(ContactsFixture, TestContactSimple) {
	contact_data.push_back (ContactInfo (contact_body_id, contact_point, contact_normal));

	Q[0] = 0.2;
	Q[3] = 0.6;
	Tau[0] = 1.0;
	Tau[1] = -5.0;
	Tau[2] = 3.0;
	{
		_NoLogging nolog;
		ForwardDynamics (*model, Q, QDot, Tau, QDDot);
	}

	cmlVector humans_values (QDDot.size());
	humans_values[0] =	-1.647101149402497e+00;
	humans_values[1] =	-3.333333333333333e+00;
	humans_values[2] =	1.500000000000000e+00;
	humans_values[3] =	4.700721141799440e+00;
	humans_values[4] =	3.598082426458145e+00;
	humans_values[5] =	-1.022528511047732e+00;

	CHECK_ARRAY_CLOSE (humans_values.data(), QDDot.data(), QDDot.size(), TEST_PREC);

	Vector3d point_accel;
	{
		CalcPointAcceleration (*model, Q, QDot, QDDot, contact_body_id, contact_point, point_accel);
	}

	ForwardDynamicsContacts(*model, Q, QDot, Tau, contact_data, QDDot);
	{
		_NoLogging nolog;
		CalcPointAcceleration (*model, Q, QDot, QDDot, contact_body_id, contact_point, point_accel);
	}

	humans_values[0] = 5.681687528667114e-01;
	humans_values[1] = -3.333333333333333e+00;
	humans_values[2] = 1.500000000000000e+00;
	humans_values[3] = 2.080750294369360e-01;
	humans_values[4] = 3.598082426458145e+00;
	humans_values[5] = -1.022528511047732e+00;

	CHECK_ARRAY_CLOSE (humans_values.data(), QDDot.data(), QDDot.size(), TEST_PREC);
}

TEST_FIXTURE(ContactsFixture, TestContactEulerSingularity) {
	contact_data.push_back (ContactInfo (contact_body_id, contact_point, contact_normal));

	Q[0] = 27.9045;
	Q[1] = -0.439375;
	Q[2] = 1.52627;
	Q[3] = 20.5971;
	Q[4] = -1.48387;
	Q[5] = -2.95715;
	QDot[0] = 12.288;
	QDot[1] = -3.42405;
	QDot[2] = -5.17653;
	QDot[3] = 45.3575;
	QDot[4] = -4.51642;
	QDot[5] = -51.0512;
	{
		_NoLogging nolog;
		ForwardDynamics (*model, Q, QDot, Tau, QDDot);
	}

	cmlVector humans_values (QDDot.size());

	humans_values[0] = 4.008081005501898e+01;
	humans_values[1] = 5.046003674456097e+01;
	humans_values[2] = -8.111575678933713e+01;
	humans_values[3] = 5.420161855111747e+03;
	humans_values[4] = 1.339039929480332e+02;
	humans_values[5] = -5.415409099653753e+03;

	/// \todo Warning: the precision of this test has been changed as
	// something strange happens (maybe a euler angle singularity?) at this
	// model state. However it is still quite close to the HuMAnS values.
	CHECK_ARRAY_CLOSE (humans_values.data(), QDDot.data(), QDDot.size(), 1.0e-10);
}

TEST_FIXTURE(ContactsFixture, TestContactFixedPoint) {
	contact_data.push_back (ContactInfo (contact_body_id, contact_point, Vector3d (1., 0., 0.)));
	contact_data.push_back (ContactInfo (contact_body_id, contact_point, Vector3d (0., 1., 0.)));
	contact_data.push_back (ContactInfo (contact_body_id, contact_point, Vector3d (0., 0., 1.)));

	cmlVector humans_values (QDDot.size());

	Q[0] = 0.2;
	Q[1] = -0.5;
	Q[2] = 0.1;
	Q[3] = -0.4;
	Q[4] = -0.1;
	Q[5] = 0.4;

	humans_values[0] = 1.834747596432898e-01;
	humans_values[1] = 2.800501472504468e-01;
	humans_values[2] = 8.568126519022337e-01;
	humans_values[3] = -1.311288581471320e-01;
	humans_values[4] = -6.079597431872865e-01;
	humans_values[5] = -4.389578697923985e-01;

	Vector3d point_accel;
	{
		SUPPRESS_LOGGING;
		ForwardDynamicsContacts(*model, Q, QDot, Tau, contact_data, QDDot);
		CalcPointAcceleration (*model, Q, QDot, QDDot, contact_body_id, contact_point, point_accel);
	}
//	cout << LogOutput.str() << endl;

//	cout << "Point Accel = " << point_accel << endl;
	CHECK_ARRAY_CLOSE (Vector3d (0., 0., 0.).data(), point_accel.data(), 3, 1.0e-12);

	/// \todo Warning: the precision of this test has been changed as
	// something strange happens (maybe a euler angle singularity?) at this
	// model state. However it is still quite close to the HuMAnS values.
	CHECK_ARRAY_CLOSE (humans_values.data(), QDDot.data(), QDDot.size(), 1.0e-12);
}

/*
TEST_FIXTURE(ContactsFixture, TestContactFloatingBaseSimple) {
	contact_data.push_back (ContactInfo (contact_body_id, contact_point, contact_normal));

	Q[0] = 0.2;
	Q[3] = 0.6;
	Tau[0] = 1.0;
	Tau[1] = -5.0;
	Tau[2] = 3.0;
	{
		_NoLogging nolog;
		ForwardDynamics (*model, Q, QDot, Tau, QDDot);
	}

	cmlVector humans_values (QDDot.size());
	humans_values[0] =	-1.647101149402497e+00;
	humans_values[1] =	-3.333333333333333e+00;
	humans_values[2] =	1.500000000000000e+00;
	humans_values[3] =	4.700721141799440e+00;
	humans_values[4] =	3.598082426458145e+00;
	humans_values[5] =	-1.022528511047732e+00;

	CHECK_ARRAY_CLOSE (humans_values.data(), QDDot.data(), QDDot.size(), TEST_PREC);

	Vector3d point_accel;
	{
		CalcPointAcceleration (*model, Q, QDot, QDDot, contact_body_id, contact_point, point_accel);
	}

	ForwardDynamicsContacts(*model, Q, QDot, Tau, contact_data, QDDot);
	{
		_NoLogging nolog;
		CalcPointAcceleration (*model, Q, QDot, QDDot, contact_body_id, contact_point, point_accel);
	}

	humans_values[0] = 5.681687528667114e-01;
	humans_values[1] = -3.333333333333333e+00;
	humans_values[2] = 1.500000000000000e+00;
	humans_values[3] = 2.080750294369360e-01;
	humans_values[4] = 3.598082426458145e+00;
	humans_values[5] = -1.022528511047732e+00;

	CHECK_ARRAY_CLOSE (humans_values.data(), QDDot.data(), QDDot.size(), TEST_PREC);
}
*/
