/*   Programmer: Jackson Rankin
 *         Date: February 24th, 2026
 * Contact info: ran23008@byui.edu
 *
 *     Overview: This file contains my own math library designed for my projects. It includes vector and matrix classes,
 *               as well as various mathematical functions that are commonly used in physics simulations. The Vec3 class
 *               represents a 3D vector, while the Vec4 class represents a 4D vector (useful for spacetime
 *               calculations). The Matrix class is a general-purpose matrix class that can be used for transformations
 *               and other linear algebra operations.
 *
 *               The library is designed to be efficient and easy to use, with operator overloads for common arithmetic
 *               operations, as well as functions for calculating angles, distances, and other properties of vectors. It
 *               also includes error handling for cases like division by zero or near-zero magnitudes to prevent
 *               numerical instability in simulations.
 *
 */

#pragma once

// Standard library header files, arranged alphabetically for readability
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace Math {
    struct Vec3 {
        public:
            /// Holds the Cartesian coordinates of the vector.
            double X, Y, Z;

            /// Holds the minimum value a double can be before we define it as 0. this is to avoid floating-point errors
            static constexpr double epsilon = 1e-12;
            static constexpr double epsilon2 = 1e-6;  // Some comparisons are of the square of the value

            // --------------------------------------------- Constructors ----------------------------------------------
            /// Takes three arguments (the Cartesian coordinates of the vector) and initializes them directly
            constexpr Vec3(double x, double y, double z) noexcept : X(x), Y(y), Z(z) {};

            /// Default construction produces a zero-vector
            constexpr Vec3() noexcept : X(0.0), Y(0.0), Z(0.0) {};

            // ------------------------------------------- Vector Properties -------------------------------------------
            [[nodiscard]] constexpr double MagnitudeSquared() const noexcept {
                /// Returns the magnitude of the vector squared.
                return X*X + Y*Y + Z*Z;
            }

            [[nodiscard]] constexpr double Magnitude() const noexcept {
                /// Returns the magnitude of the vector.
                return std::sqrt(MagnitudeSquared());
            }

            constexpr Vec3& Normalize() noexcept {
                /// This method will change the current vector into a unit vector
                double m = Magnitude();
                if (m > epsilon) {  // Magnitude check. Don't want floating-point errors to manifest in any simulation
                    X /= m; Y /= m; Z /= m;
                }
                return *this;
            }

            [[nodiscard]] constexpr Vec3 Normalized() const noexcept {
                /// This method will return a new Vec3 object, as opposed to changing the current one
                double m = Magnitude();
                if (m < epsilon) return {0, 0, 0};  // Avoid floating point errors by returning the zero vector
                return {X / m, Y / m, Z / m};
            }

            [[nodiscard]] constexpr double operator[](size_t i) const {
                /// This method will allow the user to access each component by index
                assert(i < 3);
                return (&X)[i];
                throw std::out_of_range("Index out of bounds");
            }

            [[nodiscard]] constexpr double& operator[](size_t i) {
                /// This method will allow the user to access and set each component by index
                assert(i < 3);
                return (&X)[i];
                throw std::out_of_range("Index out of bounds");
            }

            // ------------------------------------------- Angle Conversions -------------------------------------------
            [[nodiscard]] constexpr static double DegToRad(double deg) noexcept {
                /// This function takes degrees and returns the equivalent in radians
                return deg * M_PI / 180.0;
            }

            [[nodiscard]] constexpr static double RadToDeg(double rad) noexcept {
                /// This function takes radians and returns the equivalent in degrees
                return rad * 180.0 / M_PI;
            }

            // ------------------------------------------- Vector Conversions ------------------------------------------
            [[nodiscard]] constexpr static Vec3 FromCylindrical(double rho, double phi, double z) noexcept {
                /// This function takes cylindrical values, and returns a new Vec3 in Cartesian coordinates

                // Calculate the components
                double x_comp = rho * std::cos(phi);
                double y_comp = rho * std::sin(phi);
                double z_comp = z;

                return Vec3(x_comp, y_comp, z_comp);
            }

            [[nodiscard]] constexpr static Vec3 FromSpherical(double rho, double theta, double phi) noexcept{
                /// This function takes spherical values, and returns a new Vec3 in Cartesian coordinates

                // Calculate the components
                double x_comp = rho * std::sin(theta) * std::cos(phi);
                double y_comp = rho * std::sin(theta) * std::sin(phi);
                double z_comp = rho * std::cos(theta);

                return Vec3(x_comp, y_comp, z_comp);
            }

            [[nodiscard]] constexpr static std::tuple<double, double, double> ToCylindrical(const Vec3& v) noexcept {
                /// This function is intended not to produce a vector, as you would not be able to take advantage of any
                /// of the math here, so instead I designed it to return a tuple

                // Calculate the components
                double rho_comp = std::sqrt(v.X * v.X + v.Y * v.Y);
                double phi_comp = std::atan2(v.Y, v.X);
                double z_comp = v.Z;

                return std::make_tuple(rho_comp, phi_comp, z_comp);
            }

            [[nodiscard]] constexpr static std::tuple<double, double, double> ToSpherical(const Vec3& v) noexcept {
                /// This function is intended not to produce a vector, as you would not be able to take advantage of any
                /// of the math here, so instead I designed it to return a tuple

                // Calculate the rho squared
                double rho_comp2 = v.MagnitudeSquared();

                // Check if the radial component is too small
                if (rho_comp2 < epsilon2) return std::make_tuple(0.0, 0.0, 0.0);

                // Calculate the components
                double rho_comp = v.Magnitude();
                double theta_comp = std::acos(std::clamp(v.Z / rho_comp, -1.0, 1.0));
                double phi_comp = std::atan2(v.Y, v.X);

                return std::make_tuple(rho_comp, theta_comp, phi_comp);
            }

            // -------------------------------------------- Relational Math --------------------------------------------
            [[nodiscard]] constexpr static double AngleBetween(const Vec3& a, const Vec3& b) {
                /// This method calculates the angle between any two vectors, and throws an error if either have
                /// near-zero magnitudes.

                // Check if either magnitudes are too small. Remember that because we are checking MagnitudeSquared, we
                // must compare it with epsilon*epsilon, or 1e-6.
                if (a.MagnitudeSquared() < epsilon2 || b.MagnitudeSquared() < epsilon2) {
                    // If the magnitudes are too small, throw an error.
                    throw std::runtime_error("Near-zero vector magnitude.");
                }

                // Stores the vector magnitudes.
                double magA = a.Magnitude();
                double magB = b.Magnitude();

                // Calculate the cosine of the angle, then clamp it to [-1, 1]
                double cosAngle = std::clamp(Dot(a, b) / magA / magB, -1.0, 1.0);

                // Calculate and return the angle
                return std::acos(cosAngle);
            }

            [[nodiscard]] constexpr static double DistanceSquared(const Vec3& a, const Vec3& b) noexcept {
                /// Returns the distance squared between two vectors (avoids std::sqrt)
                return (a-b).MagnitudeSquared();
            }

            [[nodiscard]] constexpr static double DistanceBetween(const Vec3& a, const Vec3& b) noexcept {
                /// Returns the distance between two vectors
                return (a-b).Magnitude();
            }

            [[nodiscard]] constexpr friend bool operator==(const Vec3& a, const Vec3& b) noexcept {
                /// Determines if two vectors are within tolerance, which is 1e-12.

                // Checks every single difference between every component of the vector
                return (std::abs(a.X - b.X) < epsilon) &&
                    (std::abs(a.Y - b.Y) < epsilon) &&
                    (std::abs(a.Z - b.Z) < epsilon);
            }

            [[nodiscard]] constexpr friend bool operator!=(const Vec3& a, const Vec3& b) noexcept {
                /// Determines if two vectors aren't within tolerance
                return !(a == b);
            }

            // ------------------------------------------- Vector Arithmetic -------------------------------------------
            [[nodiscard]] constexpr friend Vec3 operator+(const Vec3& a, const Vec3& b) noexcept {
                /// Add each component from the first vector, with each respective component in the second vector.
                return Vec3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
            }

            [[nodiscard]] constexpr friend Vec3 operator-(const Vec3& a, const Vec3& b) noexcept{
                /// Subtract each component from the first vector, with each respective component in the second vector.
                return Vec3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
            }

            [[nodiscard]] constexpr friend Vec3 operator-(const Vec3& v) noexcept {
                /// Return the negative of a vector
                return Vec3(-v.X, -v.Y, -v.Z);
            }

            [[nodiscard]] constexpr friend Vec3 operator*(const Vec3& v, const double& s) noexcept {
                /// Multiplies each component of the vector by the scalar
                return Vec3(s*v.X, s*v.Y, s*v.Z);
            }

            [[nodiscard]] constexpr friend Vec3 operator*(const double& s, const Vec3& v) noexcept {
                /// Allows for commutativity in multiplication
                return Vec3(s*v.X, s*v.Y, s*v.Z);
            }

            [[nodiscard]] constexpr friend Vec3 operator/(const Vec3& v, const double& s) {
                /// Standard scalar division.

                // Check if the scalar is too small, such that the vector would blow up. Remember, epsilon = 1e-12
                if (std::abs(s) < epsilon) {
                    // Throws a fit if the absolute value of the provided scalar is below tolerance.
                    throw std::runtime_error("Division by near-zero scalar");
                }

                // Otherwise, return the quotient
                return v * (1.0 / s);
            }

            constexpr Vec3& operator+=(const Vec3& other) noexcept {
                /// Allows for addition assignment for vector objects
                X += other.X;
                Y += other.Y;
                Z += other.Z;

                return *this;
            }

            constexpr Vec3& operator-=(const Vec3& other) noexcept {
                /// Allows for subtraction assignment for vector objects
                X -= other.X;
                Y -= other.Y;
                Z -= other.Z;

                return *this;
            }

            [[nodiscard]] constexpr static double Dot(const Vec3& a, const Vec3& b) noexcept {
                /// This function returns the dot product between two vectors as a double
                return a.X*b.X + a.Y*b.Y + a.Z*b.Z;
            }

            [[nodiscard]] constexpr static Vec3 Cross(const Vec3& a, const Vec3& b) noexcept {
                /// This function returns the Cross product between two vectors

                // Calculate each component
                double x_comp = a.Y*b.Z - a.Z*b.Y;
                double y_comp = a.Z*b.X - a.X*b.Z;
                double z_comp = a.X*b.Y - a.Y*b.X;

                return Vec3(x_comp, y_comp, z_comp);
            }

            // ------------------------------------------- Vector Debugging --------------------------------------------
            friend std::ostream& operator<<(std::ostream& os, const Vec3& v) noexcept {
                /// Allow easier debugging. Operator overload, so things like std::cout << Vec3(); is valid syntax.

                // Width for each component
                constexpr int w = 12;    // adjust as needed
                constexpr int p = 6;     // digits after decimal

                // Write the tabular data to the console.
                os << "| "
                << std::setw(w) << std::fixed << std::setprecision(p) << v.X << " "
                << std::setw(w) << std::fixed << std::setprecision(p) << v.Y << " "
                << std::setw(w) << std::fixed << std::setprecision(p) << v.Z
                << " |";

                /*
                SAMPLE OUTPUT:

                |   -69.489611   -20.994045    96.062727 |
                |     0.707107     0.707107     0.000000 |

                */

                return os;
            }
    };

    struct Vec4 {
        double T, X, Y, Z;

        static constexpr double epsilon = 1e-12;

        [[nodiscard]] constexpr double Magnitude() const noexcept {
            /// Returns the magnitude of the vector.
            return std::sqrt(X*X + Y*Y + Z*Z);
        }

        // ----------------------------------------------- Constructors ------------------------------------------------
        // 4D zero-vector
        constexpr Vec4() noexcept : T(0.0), X(0.0), Y(0.0), Z(0.0) {}

        constexpr Vec4(double t, double x, double y, double z) noexcept
            : T(t), X(x), Y(y), Z(z) {}

        [[nodiscard]] constexpr Vec4 Normalized() const noexcept {
            /// This method will return a new Vec3 object, as opposed to changing the current one
            double m = Magnitude();
            if (m < epsilon) return {0, 0, 0, 0};  // Avoid floating point errors by returning the zero vector
            return {T / m, X / m, Y / m, Z / m};
        }
        // ------------------------------------------------- Indexing --------------------------------------------------
        constexpr double& operator[](size_t i) noexcept
        {
            assert(i < 4);
            return (&T)[i];
        }

        constexpr const double& operator[](size_t i) const noexcept
        {
            assert(i < 4);
            return (&T)[i];
        }

        // ------------------------------------------------- Unary ops -------------------------------------------------
        constexpr Vec4 operator-() const noexcept
        {
            return {-T, -X, -Y, -Z};
        }
        // ------------------------------------------------ Arithmetic -------------------------------------------------

        friend constexpr Vec4 operator+(const Vec4& a, const Vec4& b) noexcept
        {
            return {a.T + b.T, a.X + b.X, a.Y + b.Y, a.Z + b.Z};
        }

        friend constexpr Vec4 operator-(const Vec4& a, const Vec4& b) noexcept
        {
            return {a.T - b.T, a.X - b.X, a.Y - b.Y, a.Z - b.Z};
        }

        friend constexpr Vec4 operator*(const Vec4& v, double s) noexcept
        {
            return {v.T * s, v.X * s, v.Y * s, v.Z * s};
        }

        friend constexpr Vec4 operator*(double s, const Vec4& v) noexcept
        {
            return v * s;
        }

        friend constexpr Vec4 operator/(const Vec4& v, double s)
        {
            assert(std::abs(s) > epsilon);
            return v * (1.0 / s);
        }

        constexpr Vec4& operator+=(const Vec4& other) noexcept
        {
            T += other.T; X += other.X; Y += other.Y; Z += other.Z;
            return *this;
        }

        constexpr Vec4& operator-=(const Vec4& other) noexcept
        {
            T -= other.T; X -= other.X; Y -= other.Y; Z -= other.Z;
            return *this;
        }

        // ------------------------------------------------ Dot Product ------------------------------------------------
        constexpr double Dot(const Vec4& b) const noexcept { // Returns the spacial dot product
            return X*b.X + Y*b.Y + Z*b.Z;
        }

        constexpr double DotEuclidean(const Vec4& b) const noexcept {
            return T*b.T + X*b.X + Y*b.Y + Z*b.Z;
        }

        constexpr double DotMinkowski(const Vec4& b) const noexcept {
            return -T*b.T + X*b.X + Y*b.Y + Z*b.Z;
        }

        // ---------------------------------------------- Spatial helpers ----------------------------------------------

        // Extract spatial part as Vec3 if you want later
        constexpr auto Spatial() const noexcept
        {
            return std::tuple{X, Y, Z};
        }

        // ----------------------------------------------- Debug print -------------------------------------------------

        friend std::ostream& operator<<(std::ostream& os, const Vec4& v)
        {
            return os << "("
                      << v.T << ", "
                      << v.X << ", "
                      << v.Y << ", "
                      << v.Z << ")";
        }
    };

    class Matrix {
        private:
            size_t rows;
            size_t cols;
            std::vector<double> data; // Single contiguous block of memory for efficiency

        public:
            static constexpr double epsilon = 1e-12;
            static constexpr double epsilon2 = 1e-6;

            // --------------------------------------------- Constructors ----------------------------------------------

            // Default, 1x1, initializing each index with 0.0
            Matrix() : rows(1), cols(1), data(1, 0.0) {}

            // NxN, initializing each index with a value of 0.0
            Matrix(size_t r, size_t c, double init = 0.0)
                : rows(r), cols(c), data(r * c, init) {}


            // 4x4 constructor, initialized with custom values at each index
            Matrix(std::initializer_list<double> vals)
            {
                rows = 4;
                cols = 4;
                data.assign(16, 0.0);          // always allocate the full 4x4
                size_t i = 0;
                for (double v : vals)
                    if (i < 16) data[i++] = v; // fill however many were provided
            }

            // ------------------------------------------- Matrix Properties -------------------------------------------

            double Determinant() const {
                if (rows != cols) throw std::invalid_argument("Matrix must be square.");

                size_t n = rows;
                // Copy data for decomposition
                std::vector<double> temp = data;
                double det = 1.0;

                for (size_t i = 0; i < n; ++i) {
                    size_t pivot = i;
                    for (size_t j = i + 1; j < n; ++j) {
                        if (std::abs(temp[j * n + i]) > std::abs(temp[pivot * n + i])) pivot = j;
                    }

                    if (pivot != i) {
                        for (size_t k = 0; k < n; ++k) std::swap(temp[i * n + k], temp[pivot * n + k]);
                        det *= -1;
                    }

                    if (std::abs(temp[i * n + i]) < epsilon) return 0.0;

                    for (size_t j = i + 1; j < n; ++j) {
                        double factor = temp[j * n + i] / temp[i * n + i];
                        for (size_t k = i + 1; k < n; ++k) {
                            temp[j * n + k] -= factor * temp[i * n + k];
                        }
                    }
                    det *= temp[i * n + i];
                }
                return det;
            }

            static Matrix Identity(size_t n) {
                Matrix res(n, n, 0.0);
                for (size_t i = 0; i < n; ++i) res(i, i) = 1.0;
                return res;
            }

            // Return the number of rows and columns.
            size_t numRows() const { return rows; }
            size_t numCols() const { return cols; }

            // 2D Accessors mapping to 1D index
            double& operator()(size_t i, size_t j)
            {
                assert(i < rows && j < cols);
                return data[i * cols + j];
            }

            const double& operator()(size_t i, size_t j) const {
                return data[i * cols + j];
            }

            // ------------------------------------------- Matrix Arithmetic -------------------------------------------

            // Matrix addition and subtraction
            friend Matrix operator+(Matrix a, const Matrix& b) {
                if (a.rows != b.rows || a.cols != b.cols) throw std::invalid_argument("Size mismatch");
                for (size_t i = 0; i < a.data.size(); ++i) a.data[i] += b.data[i];
                return a;
            }

            friend Matrix operator-(Matrix a, const Matrix& b) {
                if (a.rows != b.rows || a.cols != b.cols) throw std::invalid_argument("Size mismatch");
                for (size_t i = 0; i < a.data.size(); ++i) a.data[i] -= b.data[i];
                return a;
            }

            // Scalar multiplication and division
            friend Matrix operator*(Matrix mat, double scalar) noexcept {
                for (double& val : mat.data) val *= scalar;
                return mat;
            }

            friend Matrix operator*(double scalar, Matrix mat) noexcept {
                return mat * scalar;
            }

            friend Matrix operator/(Matrix mat, double scalar) {
                if (std::abs(scalar) < epsilon) throw std::runtime_error("Division by zero");
                return mat * (1.0 / scalar);
            }

            // Vector multiplication and matrix multiplication
            friend Vec4 operator*(const Matrix& M, const Vec4& v) {
                if (M.numRows() != 4 || M.numCols() != 4)
                    throw std::invalid_argument("Matrix must be 4x4");

                return Vec4(
                    M(0,0)*v.T + M(0,1)*v.X + M(0,2)*v.Y + M(0,3)*v.Z,
                    M(1,0)*v.T + M(1,1)*v.X + M(1,2)*v.Y + M(1,3)*v.Z,
                    M(2,0)*v.T + M(2,1)*v.X + M(2,2)*v.Y + M(2,3)*v.Z,
                    M(3,0)*v.T + M(3,1)*v.X + M(3,2)*v.Y + M(3,3)*v.Z
                );
            }

            // Matrix * Matrix
            friend Matrix operator*(const Matrix& A, const Matrix& B)
            {
                if (A.cols != B.rows)
                    throw std::invalid_argument("Matrix multiply size mismatch");

                Matrix result(A.rows, B.cols, 0.0);

                for (size_t i = 0; i < A.rows; ++i)
                    for (size_t j = 0; j < B.cols; ++j)
                        for (size_t k = 0; k < A.cols; ++k)
                            result(i,j) += A(i,k) * B(k,j);

                return result;
            }
    };

}
