#include <mpi.h>//mpi
#include <iostream>
#include <vector>//container  for dynamic arrays
// Merge function
void merge(std::vector<int>& arr, std::vector<int>& left, std::vector<int>& right) {
    int i = 0, j = 0, k = 0;
    //Loop works as long as elements in both left and right arrays
    while (i < left.size() && j < right.size()) {
        //sorting 
        // compare elemnets of left and right to ensure elements added in  sorted order
        if (left[i] < right[j]) {
            arr[k++] = left[i++];
        }
        else {
            //right <left
            arr[k++] = right[j++];
        }
    }
    //Copying remaining element
    //After 1st loop ,one of subarray might remain so copy elements to arr 
    //ensuring all elements are merged finally
    while (i < left.size()) arr[k++] = left[i++];
    while (j < right.size()) arr[k++] = right[j++];
}

// MergeSort function
//Recursively sort an array using divide and conquer approach
void mergeSort(std::vector<int>& arr) {
    if (arr.size() <= 1) return;//if less than or =1 ,already soteed
    int mid = arr.size() / 2;//divdie arr into two halves(left and right)
    //sorting
    std::vector<int> left(arr.begin(), arr.begin() + mid);
    std::vector<int> right(arr.begin() + mid, arr.end());
    //by recursive
    //sort two halves
    mergeSort(left);
    mergeSort(right);
    //merge two sorted halves by using merge function
    merge(arr, left, right);
}
//Main
int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);//MPI Intializaton, All MPI programs must start with this call.

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);//rank(ID) of processsors
    MPI_Comm_size(MPI_COMM_WORLD, &size);//total number of processors

    int n;// Size of I/P the array
    std::vector<int> global_array;//Holds the array elements (used only by the root process).
    //Root Process Logic
    //Gets input from the user (array size n and its elements).
    //& Prints the unsorted array.
    if (rank == 0) {
        std::cout << "Enter the number of elements in the array: ";
        std::cin >> n;
        //entering elemnts of array
        global_array.resize(n);
        std::cout << "Enter " << n << " elements for the array:\n";
        for (int i = 0; i < n; ++i) {
            std::cin >> global_array[i];
        }
        //printing elements unsorted user entered
        std::cout << "Unsorted array: ";
        for (int num : global_array) std::cout << num << " ";
        std::cout << std::endl;
    }

    // Broadcast size of the array
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);//The array size n is broadcast from the root process (rank 0) to all other processes.
    // Ensures all processes know the size of the global array to be sorted.

    //Divde array  into chunks for each process
    // To ensure load balancing, the array is divided as evenly as possible
    // Calculate local sizes
    int base_size = n / size;//gives the basic size each process will handle.
    int remainder = n % size;//ccounts for the extra elements when n isn't divisible by the number of processes.
    std::vector<int> send_counts(size);// stores the number of elements each process will receive.
    std::vector<int> displacements(size);//calculates the starting index in the global array for the chunk assigned to process i.

    for (int i = 0; i < size; ++i) {
        send_counts[i] = base_size + (i < remainder ? 1 : 0);
        displacements[i] = (i == 0) ? 0 : (displacements[i - 1] + send_counts[i - 1]);
    }

    // Allocate local array
    std::vector<int> local_array(send_counts[rank]);

    // Data Dsistribution ,Divides the array into chunck and distributes them among processors
    MPI_Scatterv(global_array.data(), send_counts.data(), displacements.data(), MPI_INT,
        local_array.data(), send_counts[rank], MPI_INT, 0, MPI_COMM_WORLD);

    // Perform local sort
    mergeSort(local_array);

    // Collects the sorted chunks back to root
    MPI_Gatherv(local_array.data(), send_counts[rank], MPI_INT,
        global_array.data(), send_counts.data(), displacements.data(), MPI_INT, 0, MPI_COMM_WORLD);

    // Final merge step at root
    if (rank == 0) {
        int step = send_counts[0];
        for (int i = 1; i < size; ++i) {
            std::vector<int> temp(global_array.begin(), global_array.begin() + step + send_counts[i]);
            std::vector<int> right(global_array.begin() + step, global_array.begin() + step + send_counts[i]);
            merge(temp, temp, right);
            std::copy(temp.begin(), temp.end(), global_array.begin());
            step += send_counts[i];
        }

        // Output sorted array
        std::cout << "Sorted array: ";
        for (int num : global_array) std::cout << num << " ";
        std::cout << std::endl;
    }

    MPI_Finalize();//End of MPI Program
    return 0;
}
