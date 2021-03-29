#include<iostream>
#include<algorithm>
#include<cmath>
#include <random>
#include <fstream>
// #include <quadmath.h>
#include "/home/kapilv/PhD_Acads/learned_hash/PGM-index/include/pgm/pgm_index.hpp"
#include <cstring>
using namespace std;



void get_slope_bias(vector<double> &v_int,int start,int end,double &slope,double &bias,double factor)
{
  double X=0.0,Y=0.0,XX=0.0,XY=0.0;

  for(int i=start;i<end;i++)
  {
    double x=(v_int[i]-v_int[start])*1.00;
    double y=(i-start)*1.00*factor;
    X+=x;
    Y+=y;
    XX+=x*x;
    XY+=x*y;
  }
  double n_val=end-start;
  slope=(n_val*XY-X*Y)*1.00/(n_val*XX-X*X);
  bias=(Y-slope*X)*1.00/n_val;

  return ;
}



int main(int argc, char** argv){

  std::default_random_engine generator;
  std::uniform_real_distribution<double> distribution(0.0,1.0);

  // long long num_elements=2000000;

  // vector<double> v(num_elements,0.0);

  // for (int i=0; i<num_elements; i++) 
  // {
  //   v[i] = distribution(generator);
  // }

  std::vector<double> scores;
  cout<<"reading file "<<argv[1]<<endl;
  string file_name;
  string arg1(argv[1]);

  double over_alloc=stoi(argv[2]);
  int bucket_size=stoi(argv[3]);

  

  int c_check=0;
  if (arg1.compare("longitude")==0)
  {
    c_check=1;
    cout<<"longitude"<<endl;
    file_name= "data/longitude.csv";  
  }
  if (arg1.compare("genome")==0)
  {
    c_check=1;
    cout<<"genome"<<endl;
    file_name= "data/genome_data_small.csv";  
  }
  if(arg1.compare("seq")==0)
  {
    c_check=1;
    cout<<"sequential"<<endl;
    file_name= "data/seq.csv"; 
    for(int i=0;i<100000000;i++)
    {
      scores.push_back(i);
    }
  }
  if(arg1.compare("random")==0)
  {
    c_check=1;
    cout<<"random"<<endl;
    file_name= "data/random.csv"; 
    for(int i=0;i<2000000;i++)
    {
      scores.push_back(distribution(generator));
    }
  }

  std::ifstream ifile(file_name, std::ios::in);
  

  //check to see that the file was opened correctly:
  if (!ifile.is_open()) {
      std::cerr << "There was a problem opening the input file!\n";
      // exit(1);//exit or do additional error checking
  }
  else
  {
    double num = 0.0;
    //keep storing values from the text file so long as data exists:
    while (ifile >> num) {
        scores.push_back(num);
    }
  }




  cout<<"done reading file: "<<scores.size()<<endl;

  uint64_t file_elements=scores.size();

  std::sort(scores.begin(), scores.end());
  scores.erase( unique( scores.begin(), scores.end() ), scores.end() );

  file_elements=scores.size();

  cout<<"done sorting de duplicating array "<<scores.size()<<" max value is: "<<log2(scores[scores.size()-1])<<" max value is: "<<log2(scores[scores.size()-2])<<endl;

  int batch_size=100;
  
  double factor=over_alloc*1.00/bucket_size;
  long long num_loops=scores.size()/batch_size;
  double slope,bias;
  long long cur_index;
  int bit_vetor_size=batch_size*1.00*factor;
  vector<int> bit_val(bit_vetor_size,0);
  double set_count=0;

  double stretch_count=0,ptr_count=0,total_stretch=0,count_stuff=0,verify_count=0;

  num_loops--;

  cout<<"Parameter values!! "<<" bucket_size = "<<bucket_size<<" over alloc : "<<over_alloc<<endl;
  cout<<"factor: "<<factor<<" bit vector size: "<<bit_vetor_size<<endl;


  for(int i=0;i<num_loops;i++)
  {
    for(int j=0;j<bit_vetor_size;j++)
    {
      bit_val[j]=0;
    }

    cur_index=i*batch_size;

    get_slope_bias(scores,cur_index,cur_index+batch_size,slope,bias,factor);
    
    for(int j=cur_index;j<cur_index+batch_size;j++)
    {
      double index_doub=slope*(scores[j]-scores[cur_index])+bias;
      int index=index_doub;
      index=min(bit_vetor_size-1,index);
      index=max(0,index);
      bit_val[index]+=1;
    }

    long long local_count=0;

    for(int j=0;j<bit_vetor_size;j++)
    {
      local_count+=min(bit_val[j],bucket_size);
    }
    set_count+=local_count;

    stretch_count+=(batch_size-local_count);
    ptr_count+=1;

    for(int j=cur_index;j<cur_index+batch_size;j++)
    {
      if((j*17*19)%97==0)
      {
        // cout<<"lolkapil: "<<j<<" "<<scores[j+1]-scores[j]<<" "<<slope*(scores[j+1]-scores[j])<<" "<<endl;
      }
      
    }

    if(ptr_count>=10)
    {
      if(stretch_count<440)
      {
        total_stretch+=ptr_count*batch_size;
      }
      verify_count+=stretch_count;
      count_stuff+=1;
      // cout<<"lol: "<<count_stuff<<" "<<stretch_count*1.00/(batch_size*10.00)<<" "<<i<<endl;
      ptr_count=0;
      stretch_count=0;
    }


    // cout.precision(10);
    // cout<<i<<" batch empty spaces"<<batch_size-local_count<<" bound: "<<scores[cur_index]<<" "<<scores[cur_index+batch_size]<<endl;

  }

  cout<<"total favourable stretch: "<<total_stretch*1.00/scores.size()<<endl;
  cout<<"verify count: "<<verify_count*1.00/scores.size()<<endl;



  double empty_count_our=file_elements-set_count;

  cout<<"proportion collisions: "<<empty_count_our*1.00/file_elements<<endl;

  return 0;


  const int epsilon = 1; // space-time trade-off parameter
  pgm::PGMIndex<double, epsilon> index(scores);

  cout<<"done building index "<<endl;

  vector<int> set_bit(file_elements,0);
  uint64_t zero=0;

  for(int i=0;i<file_elements-100;i++)
  {
    // if(i%10000==0)
    // {
    //   cout<<i<<" batch size"<<endl;
    // }
    auto range = index.search(scores[i]);
    uint64_t val=range.pos;
    val=min(file_elements-1,val);
    val=max(zero,val);
    set_bit[range.pos]=1;
  }

  cout<<"done setting bits "<<endl;

  double count=0;

  for(int i=0;i<file_elements;i++)
  {
    count+=set_bit[i];
  }

  double empty_count=file_elements-count;

  cout<<"proportion collisions PGM: "<<empty_count*1.00/file_elements<<endl;

  cout<<"data size"<<scores.size()<<" epsilon: "<<epsilon<<endl;

  cout<<"pgm stats: "<<index.segments_count()<<" height: "<<index.height()<<" size bytes: "<<index.size_in_bytes()<<endl;


  return 0;
}