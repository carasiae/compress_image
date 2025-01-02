#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include "compress.h"

typedef struct _pixel{
    double R;
    double G;
    double B;
} pixel;

double distance_sq(pixel a, pixel b){
    return 
        (a.R - b.R) * (a.R - b.R) +
        (a.G - b.G) * (a.G - b.G) +
        (a.B - b.B) * (a.B - b.B);
}

int update_cluster(pixel *vectors, size_t length, int *cluster, pixel *centroids, int k){
    int change_count = 0;
    #pragma omp parallel for schedule(dynamic) reduction(+:change_count)
    for (int i = 0; i < length; ++i) {
        double min_distance = distance_sq(vectors[i], centroids[0]);
        int prev_cluster = cluster[i];
        int new_cluster = 0;
        for (int j = 1; j < k; ++j) {
            double distance = distance_sq(vectors[i], centroids[j]);
            if (distance < min_distance) {
                min_distance = distance;
                new_cluster = j;
            }
        }
        if (prev_cluster != new_cluster) {
                cluster[i] = new_cluster;
                ++change_count;
        }
    }
    return change_count;
}
int update_centroids(pixel *vectors, size_t length, int *cluster, pixel *centroids, int k) {
    int change_count = 0;
    pixel *temp_centroids = (pixel *)calloc(k, sizeof(pixel));
    int *counts = (int *)calloc(k, sizeof(int));
    if (!temp_centroids || !counts) {
        free(temp_centroids);
        free(counts);
        fputs("memory allocation failed", stderr);
        return -1;
    }
    #pragma omp parallel for schedule(dynamic,100)
    for (size_t j = 0; j < length; ++j) {
        int c = cluster[j];
        #pragma omp atomic
        temp_centroids[c].R += vectors[j].R;
        #pragma omp atomic
        temp_centroids[c].G += vectors[j].G;
        #pragma omp atomic
        temp_centroids[c].B += vectors[j].B;
        #pragma omp atomic
        counts[c]++;
    }
    #pragma omp parallel for schedule(static) reduction(+:change_count)
    for (int i = 0; i < k; ++i) {
        if (counts[i] > 0) {
            pixel new_centroid;
            new_centroid.R = temp_centroids[i].R / counts[i];
            new_centroid.G = temp_centroids[i].G / counts[i];
            new_centroid.B = temp_centroids[i].B / counts[i];
            if (fabs(new_centroid.R - centroids[i].R) > 1e-6 ||
                fabs(new_centroid.G - centroids[i].G) > 1e-6 ||
                fabs(new_centroid.B - centroids[i].B) > 1e-6) {
                centroids[i] = new_centroid;
                ++change_count;
            }
        }
    }
    free(temp_centroids);
    free(counts);
    return change_count;
}

/* only support 24-bit RGB data
 * length: length of pixel data in bytes
 */
void byte_to_vector(unsigned char *data, size_t length, pixel *vectors){
    for (int i = 0; i < length/3; ++i){
        vectors[i].R = (double) data[3*i];
        vectors[i].G = (double) data[3*i+1];
        vectors[i].B = (double) data[3*i+2];
    }
}

void vector_to_byte(pixel *vectors, size_t length, unsigned char *data){
    for (int i = 0; i < length; ++i){
        data[3*i] = (unsigned char) vectors[i].R;
        data[3*i+1] = (unsigned char) vectors[i].G;
        data[3*i+2] = (unsigned char) vectors[i].B;
    }
}

void collapse_cluster(pixel *vectors, int *cluster, size_t length, int k, pixel *centroids){
    for (int i = 0; i < length; ++i){
        vectors[i] = centroids[cluster[i]];
    }
}

void k_means_clustering(pixel *vectors, int *cluster, size_t length, int k, pixel *centroids, int max_iter){
    srand(time(NULL));

    // initialize centroids
    for (int i = 0; i < k; ++i){
        int valid = 0;
        pixel centroid;
        while (!valid){
            int r = rand() % length;
            centroid = vectors[r];
            valid = 1;
            for(int j = 0; j < i; j++){
                if (centroids[j].R == centroid.R
                    && centroids[j].G == centroid.G 
                    && centroids[j].B == centroid.B){
                    valid = 0;
                    break;
                }
            }
        }
        centroids[i] = centroid;
    }
    for (int i = 0; i < max_iter; ++i){
        int n_update_cluster = update_cluster(vectors, length, cluster, centroids, k);
        int n_update_centroid = update_centroids(vectors, length, cluster, centroids, k);
        if (!n_update_cluster && !n_update_centroid) {
            break;
        }
    }
}

void compress_image(unsigned char *data, size_t length, int k){
    pixel *vectors = malloc(sizeof(pixel) * length / 3);
    pixel *centroids = malloc(sizeof(pixel) * length / 3);
    int *cluster = malloc(sizeof(int) * length/3);
    if (!vectors || !centroids || !cluster){
        fputs("memory allocation failed", stderr);
        free(vectors);
        free(centroids);
        free(cluster);
    }
    byte_to_vector(data, length, vectors);
    k_means_clustering(vectors, cluster, length/3, k, centroids, 20);
    collapse_cluster(vectors, cluster, length/3, k, centroids);
    vector_to_byte(vectors, length/3, data);
    free(vectors);
    free(centroids);
    free(cluster);
}
