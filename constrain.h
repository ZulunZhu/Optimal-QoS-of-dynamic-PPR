
// Example implementation of an objective function class for linear regression
// and usage of the L-BFGS optimizer.
// 
// Compilation:
// g++ example.cpp -o example -O3 -larmadillo


#include <iostream>
#include <armadillo>
#include <ensmallen.hpp>
#include "algo.h"
#include "graph.h"
#include "heap.h"
#include "config.h"

#include<numeric>
using namespace std;
int iteration=0;
// class LinearRegressionFunction
// {
//   public:
  
//   LinearRegressionFunction(arma::mat& X, arma::vec& y) : X(X), y(y) { }
  
//   double EvaluateWithGradient(const arma::mat& theta, arma::mat& gradient)
//   {
//     const arma::vec tmp = X.t() * theta - y;
//     gradient = 2 * X * tmp;
//     return arma::dot(tmp,tmp);
//   }
  
//   private:
  
//   const arma::mat& X;
//   const arma::vec& y;
// };

class ConstrainedFunctionType
{
 public:
  //We set \Tilde{t}_q = a \beta_1\beta_2+b\beta_1+\frac{c}{\beta_1}
  // Return the objective function f(x) for the given x.
  double a,b,c,d;
  bool plot = false;
  void calculate_para(arma::mat x){

    double tau0 = qtau1*x(0,0)/config.omega;
    double epsilon = config.epsilon/config.theta;
    double tau1 = qtau2*(1-config.theta)*epsilon*config.delta*config.nodes*pow(config.alpha,2)*config.omega/
    (x(0,0)*config.edges*(x(0,1)*config.nodes*config.rbmax+1));
    double tau2 = qtau3*config.alpha*config.nodes/(x(0,0)*config.edges);
    double tau3 = utau1*x(0,1)*config.nodes*config.alpha*config.rbmax/config.edges;

    a = config.edges*(config.nodes*config.rbmax*tau1)/
    ((1-config.theta)*epsilon*config.delta*config.nodes*pow(config.alpha,2)*config.omega);

   

    b = config.edges*tau1/((1-config.theta)*epsilon*config.delta*config.nodes*pow(config.alpha,2)*config.omega)+
    config.edges*tau2/(config.alpha*config.nodes);

    c = config.omega*tau0;
  
    d = config.edges*tau3/(config.nodes*config.alpha*config.rbmax);
    
    // std::cout << "tau0 is " <<a<<" "<< b<<" "<<c<<" "<<d<<" "<<
    // " "<<endl;
  }
  // double get_function(arma::mat x){
  //   double t_q = a*x(0,0)*x(0,1)+b*x(0,0)+c/x(0,0);
  //   double t_u = d/x(0,1)+utau2;
  //   double fx = (2*(config.response_t-t_q)*(1-config.lambda_u*t_u)-config.lambda_u*(config.mv_update.second+t_u*t_u))/
  //   (config.mv_query.second+2*config.response_t*t_q-t_q*t_q);
  //   return fx;
  // }
  double Evaluate(const arma::mat& x){
    iteration++;
    double t_q = a*x(0,0)*x(0,1)+b*x(0,0)+c/x(0,0);
    double t_u = d/x(0,1)+utau2;
    double y;
    // INFO(t_q);
    // INFO(t_u);
    // if(config.test_throughput==true){
    //   // ***Optimize the lambda_q
    //   double fx = (2*(config.response_t-t_q)*(1-config.lambda_u*t_u)-config.lambda_u*(config.mv_update.second+t_u*t_u))/
    // (config.mv_query.second+2*config.response_t*t_q-t_q*t_q);
    //   y = -fx;

    // }else{
      // ***Optimize the response time
    double fx; 
    if(1-config.lambda_u*t_u-config.lambda_q*t_q>0){

      fx = (config.lambda_u*(config.mv_update.second+t_u*t_u)+config.lambda_q*(config.mv_query.second+t_q*t_q))/
      ((1-config.lambda_u*t_u-config.lambda_q*t_q)*2)+t_q;
      y = fx;
    }else if(1-config.lambda_u*t_u-config.lambda_q*t_q==0){
      fx = 1-config.lambda_u*t_u-config.lambda_q*t_q+0.1;

    }else{
      fx = 1-config.lambda_u*t_u-config.lambda_q*t_q;
      y = -fx;
    }
    // }
    
    
    
    

    

    //*** Output for visualization

    // if(plot == false){
    //   string filename = "/home/zulun/some_test/para.txt";
    //   ofstream queryfile(filename, ios::app);
    //   queryfile<<a<<"\n"<<b<<"\n"<<c<<"\n"<<d<<"\n"<<utau2<<"\n"<<config.response_t<<"\n"<<config.lambda_u
    //   <<"\n"<<config.mv_update.second<<"\n"<<config.mv_query.second<<endl;
    //   queryfile.close();
    //   plot = true;

    // }
    
    // std::cout << "Current y: " << 1-config.lambda_u*t_u-config.lambda_q*t_q<<endl;
    
    

    return y;
  };

  // Compute the gradient of f(x) for the given x and store the result in g.
  void Gradient(const arma::mat& x, arma::mat& g){
    arma::mat dbeta1,dbeta2; 
    
    double eps = 1e-8;
    dbeta1 = {x(0,0)+eps, x(0,1)};
    dbeta2 = {x(0,0), x(0,1)+eps};
    
    double t_q = a*x(0,0)*x(0,1)+b*x(0,0)+c/x(0,0);
    double t_u = d/x(0,1)+utau2;
    double fx, fxd1, fxd2;
    // if(config.test_throughput==true){
    //   // ***Optimize the lambda_q
    //   fx = (2*(config.response_t-t_q)*(1-config.lambda_u*t_u)-config.lambda_u*(config.mv_update.second+t_u*t_u))/
    //   (config.mv_query.second+2*config.response_t*t_q-t_q*t_q); 
    //   fx = -fx;
    //   double dt_q = a*dbeta1(0,0)*dbeta1(0,1)+b*dbeta1(0,0)+c/dbeta1(0,0);
    //   double dt_u = d/dbeta1(0,1)+utau2;
    //   fxd1 = (2*(config.response_t-dt_q)*(1-config.lambda_u*dt_u)-config.lambda_u*(config.mv_update.second+dt_u*dt_u))/
    //   (config.mv_query.second+2*config.response_t*dt_q-dt_q*dt_q);
    //   fxd1 = -fxd1;
    //   dt_q = a*dbeta2(0,0)*dbeta2(0,1)+b*dbeta2(0,0)+c/dbeta2(0,0);
    //   dt_u = d/dbeta2(0,1)+utau2;
    //   fxd2 = (2*(config.response_t-dt_q)*(1-config.lambda_u*dt_u)-config.lambda_u*(config.mv_update.second+dt_u*dt_u))/
    //   (config.mv_query.second+2*config.response_t*dt_q-dt_q*dt_q);
    //   fxd2 = -fxd2;
    // }else{
    
      //***Optimize the response time
    fx = (config.lambda_u*(config.mv_update.second+t_u*t_u)+config.lambda_q*(config.mv_query.second+t_q*t_q))/
    ((1-config.lambda_u*t_u-config.lambda_q*t_q)*2)+t_q;

    double dt_q = a*dbeta1(0,0)*dbeta1(0,1)+b*dbeta1(0,0)+c/dbeta1(0,0);
    double dt_u = d/dbeta1(0,1)+utau2;
    fxd1 = (config.lambda_u*(config.mv_update.second+dt_u*dt_u)+config.lambda_q*(config.mv_query.second+dt_q*dt_q))/
    ((1-config.lambda_u*dt_u-config.lambda_q*dt_q)*2)+dt_q;
    
    dt_q = a*dbeta2(0,0)*dbeta2(0,1)+b*dbeta2(0,0)+c/dbeta2(0,0);
    dt_u = d/dbeta2(0,1)+utau2;
    fxd2 = (config.lambda_u*(config.mv_update.second+dt_u*dt_u)+config.lambda_q*(config.mv_query.second+dt_q*dt_q))/
    ((1-config.lambda_u*dt_u-config.lambda_q*dt_q)*2)+dt_q;
    // }

    g = {(fxd1 - fx)/eps, (fxd2 - fx)/eps};
    // cout << "gr is " <<g<<"  "<<dbeta1<<"  "<<get_function(dbeta1)<<endl;
    // INFO(g);
    // INFO(dbeta1);
    // std::cout << "t_q,t_u is " << dt_q<< " "<<dt_u<<" "<<endl;
    // std::cout << "fxd2 is " << (2*(config.response_t-dt_q)*(1-config.lambda_u*dt_u)-config.lambda_u*(config.mv_update.second+dt_u*dt_u))
    // << " "<<(config.mv_query.second+2*config.response_t*dt_q-dt_q*dt_q)<<" "<<endl;
    // cout << "**************************************"<<endl;
  };
    
  // Get the number of constraints on the objective function.
  size_t NumConstraints(){
    return 4;
  };
  // double test(const arma::mat& x){
  //   return (1./6)*pow(x(0,0),2) + (1./6)*pow(x(0,1),2) - 1;

  // }

  // Evaluate constraint i at the parameters x.  If the constraint is
  // unsatisfied, a value greater than 0 should be returned.  If the constraint
  // is satisfied, 0 should be returned.  The optimizer will add this value to
  // its overall objective that it is trying to minimize.
  double EvaluateConstraint(const size_t i, const arma::mat& x){
    
    double t_q = a*x(0,0)*x(0,1)+b*x(0,0)+c/x(0,0);
    double t_u = d/x(0,1)+utau2;
    switch (i){
      case 0: if (config.lambda_u*t_u+config.lambda_q*t_q-1>0){
        return (config.lambda_u*t_u+config.lambda_q*t_q-1)*10;
        break;
      }
      else{
        return 0;
        break;
      }

      case 1: if (t_q-config.response_t>0){
        return (t_q-config.response_t)*100;
        break;
      }
      else{
        return 0;
        break;
      }
      case 2: if (-x(0,0)>0){
        return (-x(0,0))*100;
        break;
      }
      else{
        return 0;
        break;
      }
      case 3: if (-x(0,1)>0){
        return (-x(0,1))*100;
        break;
      }
      else{
        return 0;
        break;
      }
    }
    
  };

  // Evaluate the gradient of constraint i at the parameters x, storing the
  // result in the given matrix g.  If the constraint is not satisfied, the
  // gradient should be set in such a way that the gradient points in the
  // direction where the constraint would be satisfied.
  void GradientConstraint(const size_t i, const arma::mat& x, arma::mat& g){
    switch (i){
      case 0: g = {config.lambda_q*(a*x(0,1)+b-c/(x(0,0)*x(0,0))),config.lambda_q*a*x(0,0)-config.lambda_u*d/(x(0,1)*x(0,1))};
        break;
      case 1: g = {a*x(0,1)+b-c/(x(0,0)*x(0,0)),a*x(0,0)};
        break;
      case 2: g = {-1,0};
        break;
      case 3: g = {0,-1};
        break;
    }
  };


};

pair<double,double> improve_throughput()
{
  
  arma::mat x,original;
  x = {1.0, 1.0};
  original = x;
  ConstrainedFunctionType f;
  f.calculate_para(original);
  
  ens::AugLagrangian optimizer(1500,0.25,10);
  optimizer.Optimize(f, x);
  // output the log file
  string filename = config.graph_location + "result.txt";
  ofstream queryfile(filename, ios::app);
  char str[50];
  time_t now = time(NULL);
  strftime(str, 50, "%x %X", localtime(&now));
  queryfile << str<<endl;
  queryfile << "======Tau0 is " <<f.a<<" "<< f.b<<" "<<f.c<<" "<<f.d<<" "<<utau2<<
    " "<<endl;
  queryfile<<"Original tq is "<< f.a*original(0,0)*original(0,1)+f.b*original(0,0)+f.c/original(0,0)<<endl;
  queryfile<<"optimal tq is "<< f.a*x(0,0)*x(0,1)+f.b*x(0,0)+f.c/x(0,0)<<endl;
  queryfile << "Optimal X is " << x<< " "<<f.EvaluateConstraint(0,x)<< " "<<f.EvaluateConstraint(1,x)<< " "<<f.EvaluateConstraint(2,x)<< " "<<f.EvaluateConstraint(3,x)<< " "<<endl;
  queryfile << "original fx " << f.Evaluate(original) <<" "<<endl;
  queryfile << "Minimum of the function is " << f.Evaluate(x)<<endl;
  queryfile.close();
  if(x(0,0) <=0||x(0,1)<=0){
    x(0,0)=config.beta1;
    x(0,1)=config.beta2;
  }
  return pair<double,double>(x(0,0), x(0,1));
}
