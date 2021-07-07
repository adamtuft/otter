#define THREADS 2
#define LENGTH 20

PROGRAM omp_parallel_for
USE OMP_LIB

        integer, dimension(LENGTH) :: num
        integer :: k
        num(:) = 0

        call omp_set_num_threads(THREADS)

        !$OMP PARALLEL DO
        do k=1, LENGTH
                num(k) = omp_get_thread_num()
        end do
        !$OMP END PARALLEL DO

END PROGRAM
