#include <Core/ModelicaDefine.h>
 #include <Core/Modelica.h>
#include <Solver/UmfPack/UmfPack.h>
#include <Core/Math/ILapack.h>

#ifdef USE_UMFPACK
#include "umfpack.h"
#include <Core/Utils/numeric/bindings/umfpack/umfpack.hpp>
#include <Core/Utils/numeric/bindings/ublas/vector.hpp>
#include <Core/Utils/numeric/bindings/ublas.hpp>
#include <boost/numeric/ublas/io.hpp>
namespace umf = boost::numeric::bindings::umfpack;
#endif
UmfPack::UmfPack(IAlgLoop* algLoop, ILinSolverSettings* settings) : _iterationStatus(CONTINUE), _umfpackSettings(settings), _algLoop(algLoop), _rhs(NULL), _x(NULL), _firstuse(true), _jacd(NULL)
{
}

UmfPack::~UmfPack()
{
    if(_jacd)   delete [] _jacd;
    if(_rhs)     delete []  _rhs;
    if(_x)      delete [] _x;
}

void UmfPack::initialize()
{
#ifdef USE_UMFPACK
    _firstuse=false;
    _algLoop->initialize();
    if(_algLoop->queryDensity()<1. &&_umfpackSettings->getUseSparseFormat() )
    {
        _algLoop->setUseSparseFormat(true);

    }
    else
    {
        _jacd= new double[_algLoop->getDimReal()*_algLoop->getDimReal()];
        _algLoop->setUseSparseFormat(false);
    }


    _rhs = new double[_algLoop->getDimReal()];
    _x = new double[_algLoop->getDimReal()];
#endif
}

void UmfPack::solve()
{
#ifdef USE_UMFPACK
    if(_firstuse) initialize();
    if(!_algLoop->getUseSparseFormat())
    {
        long int dimRHS  = 1;          // Dimension of right hand side of linear system (=b)
        long int dimSys = _algLoop->getDimReal();
        long int irtrn  = 0;          // Retrun-flag of Fortran code        _algLoop->getReal(_y);
        long int * _helpArray = new long int[_algLoop->getDimReal()];
        _algLoop->evaluate();
        _algLoop->getRHS(_rhs);

        const matrix_t& A = _algLoop->getSystemMatrix();
		const double* jacd = A.data().begin();
		memcpy(_jacd, jacd, dimSys*dimSys*sizeof(double));

        dgesv_(&dimSys,&dimRHS,_jacd,&dimSys,_helpArray,_rhs,&dimSys,&irtrn);
        std::memcpy(_x,_rhs,dimSys*sizeof(double));
        _algLoop->setReal(_x);
        delete [] _helpArray;
    }
    else
    {


         int status;
        // get the default control parameters
		umf::control_type<> Control;
		// change the default print level
		Control [UMFPACK_PRL] = 6;

		 _algLoop->evaluate();
        _algLoop->getRHS(_rhs);
         long int dimSys = _algLoop->getDimReal();
        const sparsematrix_t& A = _algLoop->getSystemSparseMatrix();

        adaptor_t rhs_adaptor(dimSys,_rhs);
		shared_vector_t b(dimSys,rhs_adaptor);

        adaptor_t x_adaptor(dimSys,_x);
		shared_vector_t x(dimSys,x_adaptor);


        umf::symbolic_type<double> Symbolic;
        umf::numeric_type<double> Numeric;
		status = umf::symbolic(A, Symbolic);
        if(status<0)
			//umf::report_status (Control, status)
			throw ModelicaSimulationError(ALGLOOP_SOLVER,"Error in umfpack symbolic function");
		status = umf::numeric (A, Symbolic, Numeric);
		if(status<0)
			throw ModelicaSimulationError(ALGLOOP_SOLVER,"Error in umfpack numeric function");
        status = umf::solve (A, x, b, Numeric);
		if(status<0)
			throw ModelicaSimulationError(ALGLOOP_SOLVER,"Error in umfpack solve function");
        _algLoop->setReal(_x);


    }

#endif
}

IAlgLoopSolver::ITERATIONSTATUS UmfPack::getIterationStatus()
{
    return _iterationStatus;
}

void UmfPack::stepCompleted(double time)
{
}
