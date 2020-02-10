#include "kernels.h"
#include "cutoffs.h"
#include "local_environment.h"
#include <cmath>
#include <iostream>

TwoBodyKernel :: TwoBodyKernel() {};

TwoBodyKernel :: TwoBodyKernel(double ls, const std::string & cutoff_function,
    std::vector<double> cutoff_hyps){

    this->ls = ls;
    ls1 = 1 / (2 * ls * ls);
    ls2 = 1 / (ls * ls);
    this->cutoff_hyps = cutoff_hyps;

    if (cutoff_function == "quadratic"){
        this->cutoff_pointer = quadratic_cutoff;
    }
    else if (cutoff_function == "hard"){
        this->cutoff_pointer = hard_cutoff;
    }
    else if (cutoff_function == "cosine"){
        this->cutoff_pointer = cos_cutoff;
    }
}

// TODO: test env_env method
double TwoBodyKernel :: env_env(const LocalEnvironment & env1,
                                const LocalEnvironment & env2){
    double kern = 0;
    double ri, rj, fi, fj, rdiff;
    int e1, e2;

    double cut1 = env1.cutoff;
    double cut2 = env2.cutoff;
    double rcut_vals_1[2];
    double rcut_vals_2[2];
    int c1 = env1.central_species;
    int c2 = env2.central_species;

    for (int m = 0; m < env1.rs.size(); m ++){
        ri = env1.rs[m];
        e1 = env1.environment_species[m];
        (*cutoff_pointer)(rcut_vals_1, ri, cut1, cutoff_hyps);
        fi = rcut_vals_1[0];
        for (int n = 0; n < env2.rs.size(); n ++){
            e2 = env2.environment_species[n];

            // Proceed only if the pairs match.
            if ((c1 == c2 && e1 == e2) || (c1 == e2 && c2 == e1)){
                rj = env2.rs[n];
                (*cutoff_pointer)(rcut_vals_2, rj, cut2, cutoff_hyps);
                fj = rcut_vals_2[0];
                rdiff = ri - rj;
                kern += fi * fj * exp(-rdiff * rdiff * ls1);
            }
        }
    }
    return kern;
}

Eigen::VectorXd TwoBodyKernel :: env_struc(const LocalEnvironment & env1,
    const StructureDescriptor & struc1){
    
    int no_elements = 1 + 3 * struc1.noa + 6;
    Eigen::VectorXd kernel_vector =
        Eigen::VectorXd::Zero(no_elements);
    double ri, rj, fi, fj, fdj, rdiff, c1, c2, c3, c4, fx, fy, fz, xval, yval,
        zval, xrel, yrel, zrel, en_kern;

    int e1, e2, cent2;
    int cent1 = env1.central_species;

    double cut1 = env1.cutoff;
    double cut2 = struc1.cutoff;
    double rcut_vals_1[2];
    double rcut_vals_2[2];

    double vol_inv = 1 / struc1.volume;

    LocalEnvironment env_curr;
    for (int i = 0; i < struc1.noa; i ++){
       env_curr = struc1.environment_descriptors[i];
       cent2 = env_curr.central_species;

        for (int m = 0; m < env1.rs.size(); m ++){
            ri = env1.rs[m];
            (*cutoff_pointer)(rcut_vals_1, ri, cut1, cutoff_hyps);
            fi = rcut_vals_1[0];
            e1 = env1.environment_species[m];

            for (int n = 0; n < env_curr.rs.size(); n ++){
                e2 = env_curr.environment_species[n];

                // Proceed only if the pairs match.
                if ((cent1 == cent2 && e1 == e2) || (cent1 == e2 && 
                 cent2 == e1)){
                    rj = env_curr.rs[n];
                    rdiff = ri - rj;

                    xval = env_curr.xs[n];
                    yval = env_curr.ys[n];
                    zval = env_curr.zs[n];
                    xrel = env_curr.xrel[n];
                    yrel = env_curr.yrel[n];
                    zrel = env_curr.zrel[n];

                    (*cutoff_pointer)(rcut_vals_2, rj, cut2, cutoff_hyps);
                    fj = rcut_vals_2[0];
                    fdj = rcut_vals_2[1];

                    // energy kernel
                    c1 = rdiff * rdiff;
                    c2 = exp(-c1 * ls1);
                    kernel_vector(0) += fi * fj * c2 / 2;

                    // helper constants
                    c3 = c2 * ls2 * fi * fj * rdiff;
                    c4 = c2 * fi * fdj;

                    // fx + exx, exy, exz stress components
                    fx = xrel * c3 + c4 * xrel;
                    kernel_vector(1 + 3 * i) += fx;
                    kernel_vector(no_elements - 6) -= fx * xval * vol_inv / 2;
                    kernel_vector(no_elements - 5) -= fx * yval * vol_inv / 2;
                    kernel_vector(no_elements - 4) -= fx * zval * vol_inv / 2;
                    
                    // fy + eyy, eyz stress components
                    fy = yrel * c3 + c4 * yrel;
                    kernel_vector(2 + 3 * i) += fy;
                    kernel_vector(no_elements - 3) -= fy * yval * vol_inv / 2;
                    kernel_vector(no_elements - 2) -= fy * zval * vol_inv / 2;

                    // fz + ezz stress component
                    fz = zrel * c3 + c4 * zrel;
                    kernel_vector(3 + 3 * i) += fz;
                    kernel_vector(no_elements - 1) -= fz * zval * vol_inv / 2;
                }
            }
        } 
    }

    return kernel_vector;
}

ThreeBodyKernel :: ThreeBodyKernel() {};

ThreeBodyKernel :: ThreeBodyKernel(double ls,
    const std::string & cutoff_function, std::vector<double> cutoff_hyps){

    this->ls = ls;
    ls1 = 1 / (2 * ls * ls);
    ls2 = 1 / (ls * ls);
    this->cutoff_hyps = cutoff_hyps;

    if (cutoff_function == "quadratic"){
        this->cutoff_pointer = quadratic_cutoff;
    }
    else if (cutoff_function == "hard"){
        this->cutoff_pointer = hard_cutoff;
    }
    else if (cutoff_function == "cosine"){
        this->cutoff_pointer = cos_cutoff;
    }
}

double ThreeBodyKernel :: env_env(const LocalEnvironment & env1,
                                  const LocalEnvironment & env2){
    double kern = 0;
    double ri1, ri2, ri3, rj1, rj2, rj3, fi1, fi2, fi3, fj1, fj2, fj3,
        fdi1, fdi2, fdi3, fdj1, fdj2, fdj3, fi, fj, r11, r12, r13, r21,
        r22, r23, r31, r32, r33, p1, p2, p3, p4, p5, p6;
    int i1, i2, j1, j2, ei1, ei2, ej1, ej2;

    double cut1 = env1.cutoff;
    double cut2 = env2.cutoff;
    double rcut_vals_i1[2], rcut_vals_i2[2], rcut_vals_i3[2],
        rcut_vals_j1[2], rcut_vals_j2[2], rcut_vals_j3[2];
    int c1 = env1.central_species;
    int c2 = env2.central_species;

    for (int m = 0; m < env1.three_body_indices.size(); m ++){
        i1 = env1.three_body_indices[m][0];
        i2 = env1.three_body_indices[m][1];

        ri1 = env1.rs[i1];
        ri2 = env1.rs[i2];
        ri3 = env1.cross_bond_dists[m];

        ei1 = env1.environment_species[i1];
        ei2 = env1.environment_species[i2];

        (*cutoff_pointer)(rcut_vals_i1, ri1, cut1, cutoff_hyps);
        (*cutoff_pointer)(rcut_vals_i2, ri2, cut1, cutoff_hyps);
        (*cutoff_pointer)(rcut_vals_i3, ri3, cut1, cutoff_hyps);

        fi1 = rcut_vals_i1[0];
        fi2 = rcut_vals_i2[0];
        fi3 = rcut_vals_i3[0];
        fi = fi1 * fi2 * fi3;

        fdi1 = rcut_vals_i1[1];
        fdi2 = rcut_vals_i2[1];

        for (int n = 0; n < env2.three_body_indices.size(); n ++){
            j1 = env2.three_body_indices[n][0];
            j2 = env2.three_body_indices[n][1];

            rj1 = env2.rs[j1];
            rj2 = env2.rs[j2];
            rj3 = env2.cross_bond_dists[n];

            ej1 = env2.environment_species[j1];
            ej2 = env2.environment_species[j2];

            (*cutoff_pointer)(rcut_vals_j1, rj1, cut2, cutoff_hyps);
            (*cutoff_pointer)(rcut_vals_j2, rj2, cut2, cutoff_hyps);
            (*cutoff_pointer)(rcut_vals_j3, rj3, cut1, cutoff_hyps);

            fj1 = rcut_vals_j1[0];
            fj2 = rcut_vals_j2[0];
            fj3 = rcut_vals_j3[0];
            fj = fj1 * fj2 * fj3;

            fdj1 = rcut_vals_j1[1];
            fdj2 = rcut_vals_j2[1];

            r11 = ri1 - rj1;
            r12 = ri1 - rj2;
            r13 = ri1 - rj3;
            r21 = ri2 - rj1;
            r22 = ri2 - rj2;
            r23 = ri2 - rj3;
            r31 = ri3 - rj1;
            r32 = ri3 - rj2;
            r33 = ri3 - rj3;

            // Sum over six permutations.
            if (c1 == c2){
                if (ei1 == ej1 && ei2 == ej2){
                    p1 = r11 * r11 + r22 * r22 + r33 * r33;
                    kern += exp(-p1 * ls2) * fi * fj;
                }
                if (ei1 == ej2 && ei2 == ej1){
                    p2 = r12 * r12 + r21 * r21 + r33 * r33;
                    kern += exp(-p2 * ls2) * fi * fj;
                }
            }

            if (c1 == ej1){
                if (ei1 == ej2 && ei2 == c2){
                    p3 = r13 * r13 + r21 * r21 + r32 * r32;
                    kern += exp(-p3 * ls2) * fi * fj;
                }
                if (ei1 == c2 && ei2 == ej2){
                    p4 = r11 * r11 + r23 * r23 + r32 * r32;
                    kern += exp(-p4 * ls2) * fi * fj;
                }
            }

            if (c1 == ej2){
                if (ei1 == ej1 && ei2 == c2){
                    p5 = r13 * r13 + r22 * r22 + r31 * r31;
                    kern += exp(-p5 * ls2) * fi * fj;
                }
                if (ei1 == c2 && ei2 == ej1){
                    p6 = r12*r12+r23*r23+r31*r31;
                    kern += exp(-p6 * ls2) * fi * fj;
                }
            }

        }
    }

    return kern;
}

Eigen::VectorXd ThreeBodyKernel :: env_struc(const LocalEnvironment & env1,
    const StructureDescriptor & struc1){

    int no_elements = 1 + 3 * struc1.noa + 6;
    Eigen::VectorXd kernel_vector =
        Eigen::VectorXd::Zero(no_elements);

    double kern = 0;
    double ri1, ri2, ri3, rj1, rj2, rj3, fi1, fi2, fi3, fj1, fj2, fj3,
        fdi1, fdi2, fdi3, fdj1, fdj2, fdj3, fi, fj, r11, r12, r13, r21,
        r22, r23, r31, r32, r33, p1, p2, p3, p4, p5, p6;
    int i1, i2, j1, j2, ei1, ei2, ej1, ej2, c2;

    LocalEnvironment env2;

    double cut1 = env1.cutoff;
    double cut2 = struc1.cutoff;
    double rcut_vals_i1[2], rcut_vals_i2[2], rcut_vals_i3[2],
        rcut_vals_j1[2], rcut_vals_j2[2], rcut_vals_j3[2];
    int c1 = env1.central_species;

    for (int i = 0; i < struc1.noa; i ++){
       env2 = struc1.environment_descriptors[i];
       c2 = env2.central_species;

        for (int m = 0; m < env1.three_body_indices.size(); m ++){
            i1 = env1.three_body_indices[m][0];
            i2 = env1.three_body_indices[m][1];

            ri1 = env1.rs[i1];
            ri2 = env1.rs[i2];
            ri3 = env1.cross_bond_dists[m];

            ei1 = env1.environment_species[i1];
            ei2 = env1.environment_species[i2];

            (*cutoff_pointer)(rcut_vals_i1, ri1, cut1, cutoff_hyps);
            (*cutoff_pointer)(rcut_vals_i2, ri2, cut1, cutoff_hyps);
            (*cutoff_pointer)(rcut_vals_i3, ri3, cut1, cutoff_hyps);

            fi1 = rcut_vals_i1[0];
            fi2 = rcut_vals_i2[0];
            fi3 = rcut_vals_i3[0];
            fi = fi1 * fi2 * fi3;

            fdi1 = rcut_vals_i1[1];
            fdi2 = rcut_vals_i2[1];

            for (int n = 0; n < env2.three_body_indices.size(); n ++){
                j1 = env2.three_body_indices[n][0];
                j2 = env2.three_body_indices[n][1];

                rj1 = env2.rs[j1];
                rj2 = env2.rs[j2];
                rj3 = env2.cross_bond_dists[n];

                ej1 = env2.environment_species[j1];
                ej2 = env2.environment_species[j2];

                (*cutoff_pointer)(rcut_vals_j1, rj1, cut2, cutoff_hyps);
                (*cutoff_pointer)(rcut_vals_j2, rj2, cut2, cutoff_hyps);
                (*cutoff_pointer)(rcut_vals_j3, rj3, cut1, cutoff_hyps);

                fj1 = rcut_vals_j1[0];
                fj2 = rcut_vals_j2[0];
                fj3 = rcut_vals_j3[0];
                fj = fj1 * fj2 * fj3;

                fdj1 = rcut_vals_j1[1];
                fdj2 = rcut_vals_j2[1];

                r11 = ri1 - rj1;
                r12 = ri1 - rj2;
                r13 = ri1 - rj3;
                r21 = ri2 - rj1;
                r22 = ri2 - rj2;
                r23 = ri2 - rj3;
                r31 = ri3 - rj1;
                r32 = ri3 - rj2;
                r33 = ri3 - rj3;

                // Sum over six permutations.
                if (c1 == c2){
                    if (ei1 == ej1 && ei2 == ej2){
                        p1 = r11 * r11 + r22 * r22 + r33 * r33;
                        kern += exp(-p1 * ls2) * fi * fj;
                    }
                    if (ei1 == ej2 && ei2 == ej1){
                        p2 = r12 * r12 + r21 * r21 + r33 * r33;
                        kern += exp(-p2 * ls2) * fi * fj;
                    }
                }

                if (c1 == ej1){
                    if (ei1 == ej2 && ei2 == c2){
                        p3 = r13 * r13 + r21 * r21 + r32 * r32;
                        kern += exp(-p3 * ls2) * fi * fj;
                    }
                    if (ei1 == c2 && ei2 == ej2){
                        p4 = r11 * r11 + r23 * r23 + r32 * r32;
                        kern += exp(-p4 * ls2) * fi * fj;
                    }
                }

                if (c1 == ej2){
                    if (ei1 == ej1 && ei2 == c2){
                        p5 = r13 * r13 + r22 * r22 + r31 * r31;
                        kern += exp(-p5 * ls2) * fi * fj;
                    }
                    if (ei1 == c2 && ei2 == ej1){
                        p6 = r12*r12+r23*r23+r31*r31;
                        kern += exp(-p6 * ls2) * fi * fj;
                    }
                }

            }
        }
    }

    return kernel_vector;
}

DotProductKernel :: DotProductKernel() {};

DotProductKernel :: DotProductKernel(double signal_variance, double power){
    this->signal_variance = signal_variance;
    sig2 = signal_variance * signal_variance;
    this->power = power;
}

double DotProductKernel :: env_env(const LocalEnvironmentDescriptor & env1,
                                   const LocalEnvironmentDescriptor & env2){
    // Central species must be the same to give a nonzero kernel.
    if (env1.central_species != env2.central_species) return 0;

    double dot = env1.descriptor_vals.dot(env2.descriptor_vals);
    double d1 = env1.descriptor_norm;
    double d2 = env2.descriptor_norm;

    return pow(dot / (d1 * d2), power);
}

Eigen::VectorXd DotProductKernel
    :: env_struc(const LocalEnvironmentDescriptor & env1,
                 const StructureDescriptor & struc1){

    Eigen::VectorXd kern_vec = Eigen::VectorXd::Zero(1 + struc1.noa * 3 + 6);

    // Account for edge case where d1 is zero.
    double empty_thresh = 1e-8;
    double d1 = env1.descriptor_norm;
    if (d1 < empty_thresh) return kern_vec;

    double en_kern = 0;
    Eigen::VectorXd force_kern = Eigen::VectorXd::Zero(struc1.noa * 3);
    Eigen::VectorXd stress_kern = Eigen::VectorXd::Zero(6);

    Eigen::VectorXd force_dot, stress_dot, f1, s1;
    const double vol_inv = 1 / struc1.volume;
    double dot_val, d2, norm_dot, dval, d2_cubed;
    LocalEnvironmentDescriptor env_curr;

    for (int i = 0; i < struc1.noa; i ++){
        env_curr = struc1.environment_descriptors[i];

        // Check that the environments have the same central species.
        if (env1.central_species != env_curr.central_species) continue;

        // Check that d2 is nonzero.
        d2 = env_curr.descriptor_norm;
        if (d2 < empty_thresh) continue;
        d2_cubed = d2 * d2 * d2;

        // Energy kernel
        dot_val = env1.descriptor_vals.dot(env_curr.descriptor_vals);
        norm_dot = dot_val / (d1 * d2);
        en_kern += pow(norm_dot, power);

        // Force kernel
        force_dot = env_curr.descriptor_force_dervs * env1.descriptor_vals;
        f1 = (force_dot / (d1 * d2)) -
            (dot_val * env_curr.force_dot / (d2_cubed * d1));
        dval = power * pow(norm_dot, power - 1);
        force_kern += dval * f1;

        // Stress kernel
        stress_dot = env_curr.descriptor_stress_dervs * env1.descriptor_vals;
        s1 = (stress_dot / (d1 * d2)) -
            (dot_val * env_curr.stress_dot /(d2_cubed * d1));
        stress_kern += dval * s1;
    }

    kern_vec(0) = en_kern;
    kern_vec.segment(1, struc1.noa * 3) = -force_kern;
    kern_vec.tail(6) = -stress_kern * vol_inv;
    return kern_vec;
}
