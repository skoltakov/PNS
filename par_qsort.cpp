/*Parallel Quick Sort*/
#include <iostream>
#include <vector>
#include <time.h>
#include <string.h>
#include <mpi.h>

using namespace std;

int main(int argc, char **argv)
{
   int node, node_val, p;
   int dest = 0;
   int tag = 0;
   MPI_Status status;
   char* time_str, buffer[100];
   int position = 0;
   int str_length;
   int* local_keys;
   int* global_keys;
   int* send_offsets;
   int* receive_offsets;

   int* local_array;
   int* new_local_array;
   int* send_counts;
   int* send_displacements;
   int* recv_counts;
   int* recv_displacements;

   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &node);
   MPI_Comm_size(MPI_COMM_WORLD, &p);
   
   if (node==0) {
     time_t rawtime;
     time(&rawtime);
     time_str = ctime(&rawtime);
     str_length =  strlen(time_str);
     cout<<"Time: "<<time_str<<"length: "<<str_length<<"Last: "<<time_str[str_length-2]<<endl;

     MPI_Pack(&str_length, 1, MPI_INT, buffer, 100, &position, MPI_COMM_WORLD);  
     MPI_Pack(time_str, str_length, MPI_CHAR, buffer, 100, &position, MPI_COMM_WORLD);
     MPI_Bcast(buffer, 100, MPI_PACKED, 0, MPI_COMM_WORLD);
   }else{ 
     MPI_Bcast(buffer, 100, MPI_PACKED, 0, MPI_COMM_WORLD);
     MPI_Unpack(buffer, 100, &position, &str_length, 1, MPI_INT, MPI_COMM_WORLD);
     time_str = new char[str_length];
     MPI_Unpack(buffer, 100, &position, time_str, str_length, MPI_CHAR, MPI_COMM_WORLD);
   }

   cout<<"It's "<<time_str<<"at proc# "<<node<<endl;
   //delete[] time_str;

   MPI_Barrier(MPI_COMM_WORLD);
   
   if(node==0) global_keys = new int[p*p];
   local_keys = new int[p];
   for(int i=0;i<p;i++) local_keys[i]=p*node;

   MPI_Gather(local_keys, p, MPI_INT, global_keys, p, MPI_INT, 0, MPI_COMM_WORLD);

   if(node==0) {
     cout<<"Gathered keys on Node 0 from all Nodes: "<<endl;
     for(int i = 0; i< p*p; i++){
       cout<<global_keys[i]<<" "; 
       if (((i+1) % p)==0) cout<<endl;
     }
     delete[] global_keys;
     //set pivots on node 0
     for(int i = 0; i < p; i++) local_keys[i]=100;
    }

   // Broadcase pivots from node 0
   MPI_Bcast(local_keys, p, MPI_INT, 0, MPI_COMM_WORLD);
   cout<<"Broadcasted pivots from Node 0 on Node "<<node<<": ";
   for(int i = 0; i < p; i++) cout<<local_keys[i];
   cout<<endl;
   delete[] local_keys;

   MPI_Barrier(MPI_COMM_WORLD);

   // Alltoall exchange of sub-array offsets for each Node
   send_offsets = new int[p];
   receive_offsets = new int[p];
   for(int i = 0; i < p; i++) send_offsets[i]=node;  
   MPI_Alltoall(send_offsets, 1, MPI_INT, receive_offsets, 1, MPI_INT, MPI_COMM_WORLD);
   cout<<"Alltoall offsets on Node "<<node<<": ";
   for(int i = 0; i < p; i++) cout<<receive_offsets[i]<<" ";
   cout<<endl;
   delete[] send_offsets;
   delete[] receive_offsets;

   // Alltoall exchange of sub-array chunks with known offsets
   local_array = new int[p*(node+1)]; 
   for(int i = 0; i < p*(node+1); i++) local_array[i]=node+1; 
   send_counts = new int[p]; for(int i = 0; i < p; i++) send_counts[i]=node+1;
   send_displacements = new int[p]; 
   send_displacements[0]=0;
   for(int i = 1; i < p; i++) 
     send_displacements[i]=send_displacements[i-1]+send_counts[i-1];
   int new_size = p*(1+p)/2;cout<<"New size:"<<new_size<<endl;
   new_local_array = new int[new_size]; 
   for(int i = 0; i < new_size; i++) new_local_array[i]=0;
   recv_counts = new int[p]; for(int i = 0; i < p; i++) recv_counts[i]=i+1;
   recv_displacements = new int[p]; 
   recv_displacements[0]=0;
   for(int i = 1; i < p; i++) 
     recv_displacements[i]=recv_displacements[i-1]+recv_counts[i-1];

   MPI_Alltoallv(local_array, send_counts, send_displacements, MPI_INT,
		 new_local_array, recv_counts, recv_displacements, MPI_INT, 
		 MPI_COMM_WORLD);
   cout<<"Alltoallv new array on Node "<<node<<": ";
   for(int i = 0; i < new_size; i++) cout<<new_local_array[i]<<" ";
   cout<<endl;
   delete[] local_array;
   delete[] send_counts;
   delete[] send_displacements;
   delete[] new_local_array;
   delete[] recv_counts;
   delete[] recv_displacements;

   MPI_Barrier(MPI_COMM_WORLD);

   if (node==0) {
     vector<int> vNode(1,node);
     for(int source = 1; source < p; source++){
       MPI_Recv(&node_val, 1, MPI_INT, source, tag, MPI_COMM_WORLD, &status); 
       vNode.push_back(node_val);
     }
     for(vector<int>::iterator it=vNode.begin();it!=vNode.end();it++)
       cout<<"Hello World from Node "<<*it<<endl;
   }
   else
     MPI_Send(&node, 1, MPI_INT, dest, tag, MPI_COMM_WORLD);
   
         
   MPI_Finalize();
   return 0;
}
