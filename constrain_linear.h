
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
  double qq, uu;
  double complexity[5] = {};
  bool plot = false;
  void calculate_para(arma::mat x){
    string ab_values = config.graph_location + "linear_values.txt";
    cout<<ab_values<<endl;
    if(exists_test(ab_values)==true){
      FILE *fin = fopen(ab_values.c_str(), "r");
      char t1[100], t2[100];
      int i = 0;
      while (fscanf(fin, "%s%s", t1, t2) != EOF) {
          alist[i] = atof(t1);
          blist[i] = atof(t2);
          i++;        
      }

    }
    
    
    double epsilon = config.epsilon/config.theta;
    complexity[0] = config.omega;
    complexity[1] = (config.lambda_u*config.edges*(config.nodes*config.rbmax+1))/
    (config.lambda_q*(1-config.theta)*epsilon*config.delta*config.nodes*pow(config.alpha,2)*config.omega);
    complexity[2] = config.edges/(config.alpha);
    complexity[3] = config.edges/(config.nodes*config.alpha*config.rbmax);
    complexity[4] = config.nodes; 

    tau[0] = maxqtau1/complexity[0];
    tau[1] = maxqtau2/complexity[1];
    tau[2] = maxqtau3/complexity[2];
    tau[3] = maxutau1/complexity[3];
    tau[4] = utau2/complexity[4];

    cout<<"ab values::::::"<<alist[4]<<" "<< blist[4]<<endl;

    
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
  pair<double,double> get_qu(const arma::mat& var){
    double complexity_scale[5] = {};
    double epsilon = config.epsilon/config.theta;
    complexity_scale[0] = config.omega/var(0,0);
    complexity_scale[1] = (config.lambda_u*var(0,0)*config.edges*(var(0,1)*config.nodes*config.rbmax+1))/
    (config.lambda_q*(1-config.theta)*epsilon*config.delta*config.nodes*pow(config.alpha,2)*config.omega);
    complexity_scale[2] = (var(0,0)*config.edges)/(config.alpha);
    complexity_scale[3] = config.edges/(var(0,1)*config.nodes*config.alpha*config.rbmax);
    complexity_scale[4] = config.nodes;
    double t_q = 0;
    double t_u = 0;
    alist[4] = 1;
    blist[4] = 0;
    for(int i = 0; i < 3; i++){
      t_q = t_q+(alist[i]*complexity_scale[i]+blist[i])*tau[i];
    }
    for(int j = 3; j < 5; j++){
      t_u = t_u+(alist[j]*complexity_scale[j]+blist[j])*tau[j];
    }
    
    return pair<double,double>(t_q,t_u);

  }
  double Evaluate(const arma::mat& x){
    iteration++;
    double t_q,t_u;
    t_q = get_qu(x).first;
    t_u = get_qu(x).second;

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
    double t_q,t_u;
    t_q = get_qu(x).first;
    t_u = get_qu(x).second;
 
    double fx, fxd1, fxd2;
      //***Optimize the response time
    fx = (config.lambda_u*(config.mv_update.second+t_u*t_u)+config.lambda_q*(config.mv_query.second+t_q*t_q))/
    ((1-config.lambda_u*t_u-config.lambda_q*t_q)*2)+t_q;

  
    double dt_q,dt_u;
    dt_q = get_qu(dbeta1).first;
    dt_u = get_qu(dbeta1).second;

    fxd1 = (config.lambda_u*(config.mv_update.second+dt_u*dt_u)+config.lambda_q*(config.mv_query.second+dt_q*dt_q))/
    ((1-config.lambda_u*dt_u-config.lambda_q*dt_q)*2)+dt_q;
    
    dt_q = get_qu(dbeta2).first;
    dt_u = get_qu(dbeta2).second;
    
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
    return 3;
  };
  // double test(const arma::mat& x){
  //   return (1./6)*pow(x(0,0),2) + (1./6)*pow(x(0,1),2) - 1;

  // }

  // Evaluate constraint i at the parameters x.  If the constraint is
  // unsatisfied, a value greater than 0 should be returned.  If the constraint
  // is satisfied, 0 should be returned.  The optimizer will add this value to
  // its overall objective that it is trying to minimize.
  double EvaluateConstraint(const size_t i, const arma::mat& x){
    
    // double t_q = a*x(0,0)*x(0,1)+b*x(0,0)+c/x(0,0);
    // double t_u = d/x(0,1)+utau2;
    double t_q,t_u;
    t_q = get_qu(x).first;
    t_u = get_qu(x).second;
    switch (i){
      case 0: if (config.lambda_u*t_u+config.lambda_q*t_q-1>0){
        return (config.lambda_u*t_u+config.lambda_q*t_q-1)*1000;
        break;
      }
      else{
        return 0;
        break;
      }
      case 1: if (-x(0,0)>0){
        return (-x(0,0))*1000;
        break;
      }
      else{
        return 0;
        break;
      }
      case 2: if (-x(0,1)>0){
        return (-x(0,1))*1000;
        break;
      }
      else{
        return 0;
        break;
      }
      // case 3: if (t_q-config.response_t>0){
      //   return (t_q-config.response_t)*100;
      //   break;
      // }
      // else{
      //   return 0;
      //   break;
      // }

    }
    
  };

  // Evaluate the gradient of constraint i at the parameters x, storing the
  // result in the given matrix g.  If the constraint is not satisfied, the
  // gradient should be set in such a way that the gradient points in the
  // direction where the constraint would be satisfied.
  void GradientConstraint(const size_t i, const arma::mat& x, arma::mat& g){
    arma::mat dbeta1,dbeta2; 
    
    double eps = 1e-8;
    dbeta1 = {x(0,0)+eps, x(0,1)};
    dbeta2 = {x(0,0), x(0,1)+eps};
    double t_q,t_u;
    t_q = get_qu(x).first;
    t_u = get_qu(x).second;
 
    double fx, fxd1, fxd2, fx_r,fxd1_r, fxd2_r;
      //***Optimize the response time
    fx = config.lambda_u*t_u+config.lambda_q*t_q-1;
    fx_r = t_q-config.response_t;
  
    double dt_q,dt_u;
    dt_q = get_qu(dbeta1).first;
    dt_u = get_qu(dbeta1).second;

    fxd1 = config.lambda_u*dt_u+config.lambda_q*dt_q-1;
    fxd1_r = dt_q-config.response_t;

    dt_q = get_qu(dbeta2).first;
    dt_u = get_qu(dbeta2).second;
    fxd2 = config.lambda_u*dt_u+config.lambda_q*dt_q-1;
    fxd2_r = dt_q-config.response_t;
    switch (i){
      case 0: g = {(fxd1 - fx)/eps, (fxd2 - fx)/eps};
        break;
      // case 1: g = {(fxd1_r - fx_r)/eps, (fxd2_r - fx_r)/eps};
      //   break;
      case 1: g = {-1,0};
        break;
      case 2: g = {0,-1};
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
  
  ens::AugLagrangian optimizer(100,0.25,10);
  optimizer.Optimize(f, x);
  // output the log file
  string filename = config.graph_location + "result.txt";
  ofstream queryfile(filename, ios::app);
  char str[50];
  time_t now = time(NULL);
  strftime(str, 50, "%x %X", localtime(&now));
  queryfile << str<<endl;
  queryfile << "======Tau0 is " <<tau[0]<<" "<< tau[1]<<" "<<tau[2]<<" "<<tau[3]<<" "<<tau[4]<<
    " "<<endl;
  queryfile <<" complexity is "<<f.complexity[0]<<" "<<f.complexity[1]<<" "<<f.complexity[2]<<" "<<f.complexity[3]<<" "<<f.complexity[4]<<endl;
  queryfile<<"variance tq is "<< config.mv_query.second <<"  "<<" variance tu is "<< config.mv_update.second<<endl;
  queryfile<<"Original tq is "<< f.get_qu(original).first<<"  Original tu is "<< f.get_qu(original).second<<endl;
  queryfile<<"optimal tq is "<< f.get_qu(x).first<<"  Optimal tu is "<< f.get_qu(x).second<<endl;
  queryfile << "Constrain is " << x<< " "<<f.EvaluateConstraint(0,x)<< " "<<f.EvaluateConstraint(1,x)<< " "<<f.EvaluateConstraint(2,x)<<endl;
  queryfile << "original fx " << f.Evaluate(original) <<" "<<endl;
  queryfile << "Minimum of the function is " << f.Evaluate(x)<<endl;
  queryfile.close();
  if(x(0,0) <=0||x(0,1)<=0){
    x(0,0)=config.beta1;
    x(0,1)=config.beta2;
  }
  return pair<double,double>(x(0,0), x(0,1));
}
