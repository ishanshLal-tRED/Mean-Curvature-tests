#include <iomanip>
#include <limits>
#include <glm/gtx/norm.hpp>
#include <fstream>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <GLCore.h>
#include <GLCore/Core/Input.h>
#include <Utilities/utility.h>
using namespace GLCore;
using namespace GLCore::Utils;

bool MeanCurvatureCalculate (const char *debug_filename
							 , const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &curvature_diffuse_color
							 , std::vector<glm::vec3> &mean_curvature_normals, std::vector<float> &mean_curvature_values
							 , const std::vector<glm::vec3> &blend_betweencolors
							 , const bool trackOutput, float *save_min_mean_curvature = nullptr, float *save_max_mean_curvature = nullptr)
{
	float max_curvature = -std::numeric_limits<float>::max ();
	float min_curvature = std::numeric_limits<float>::max ();

	std::vector<glm::vec3> array_K_Xi;
	array_K_Xi.resize (posn_and_normals.size ());
	std::vector<float> array_K_h; // mean curvature value
	array_K_h.resize (posn_and_normals.size ());

#if MODE_DEBUG
	const char *fn = debug_filename != nullptr ? debug_filename : "curvatureResults.txt";
	std::ofstream ofs (fn);
	if (ofs.bad ()) {
		std::cout << "File creation error - " << fn << '\n';
		__debugbreak ();
	}
	std::mutex mutex_ofs;
#endif

	std::mutex mutex_curvature_push, mutex_cout, sync_lock, another_sync_lock;
	std::condition_variable barrier; std::unique_lock synced_lock (sync_lock);
	bool first_to_reach_access_it = true;

	std::atomic<size_t> vertices_processed = 0;
	if (trackOutput)
		std::cout << "index | A_mixed | curvature Kh |    K(Xi)\n";
	auto mean_curvature_func = [&](const uint32_t start, const uint32_t stride) {

		std::ostringstream cout_stream;
	#if MODE_DEBUG
		std::ostringstream out_stream;
		out_stream << std::fixed << std::setprecision (8);
	#endif

		for (size_t curr_indice = start; curr_indice < posn_and_normals.size (); curr_indice += stride, vertices_processed++) // repeat for every vertex
		{
			if (!trackOutput && curr_indice % (stride+1) == 0) {
				mutex_cout.lock ();
				std::cout << "\r  vertices_processed: " << vertices_processed << " out_of: " << posn_and_normals.size ();
				mutex_cout.unlock ();
			}

		#if MODE_DEBUG
			out_stream << "\nIDX: " << curr_indice << '\n';
		#endif

			glm::vec3 K_Xi = glm::vec3 (0);
			{
				std::vector<uint32_t> ring;
				{
					std::vector<uint32_t> indices_of_neighbouring_vertives; // bucket of neighboring vertices

					for (size_t i = 0; i < indices.size (); i++) { // Scan for associated primitives
						if (indices[i] == curr_indice) {
							uint32_t i_mod_3 = (i % 3),
								grp = i - i_mod_3;// note: we are assuming model is made up of triangles, so grps of 3
							indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 1)%3)]); // for anticlockwise iteration
							indices_of_neighbouring_vertives.push_back (indices[grp + ((i_mod_3 + 2)%3)]); // for anticlockwise iteration
						}
					}
					//LOG_ASSERT (indices_of_neighbouring_vertives.empty ());
					if (indices_of_neighbouring_vertives.empty ()) {
						glm::vec3 vec = posn_and_normals[curr_indice].first;
						LOG_WARN ("vertice with empty ring: {3}, [{0}, {1}, {2}]", vec.x, vec.y, vec.z, curr_indice);
						continue;
					}

					ring.push_back (indices_of_neighbouring_vertives[0]);
					ring.push_back (indices_of_neighbouring_vertives[1]);
					bool keep_adding = true;
					while (keep_adding && !indices_of_neighbouring_vertives.empty ()) {
						keep_adding = false;

						// forward chain extension
						// 0-----0 - - 0 <-to_find_this
						//       ^-is_common_vertex
						for (auto itr = indices_of_neighbouring_vertives.begin (); itr != indices_of_neighbouring_vertives.end (); itr += 2) { // every odd position
							if (ring.back () == *itr) { // found shared vertex
								ring.push_back (*(itr+1)); // extend ring
								indices_of_neighbouring_vertives.erase (itr+1);
								indices_of_neighbouring_vertives.erase (itr);
								keep_adding = true;
								break;
							}
						}
						// backward chain extension
						// to_find_this->0 - - 0-----0
						// is_common_vertex ---^
						for (size_t i = 1; i < indices_of_neighbouring_vertives.size (); i += 2) { // every odd position
							if (ring.front () == indices_of_neighbouring_vertives[i]) { // found shared vertex
								ring.insert (ring.begin (), indices_of_neighbouring_vertives[i-1]); // extend ring
								auto itr = indices_of_neighbouring_vertives.begin () + (i-1);
								indices_of_neighbouring_vertives.erase (itr+1);
								indices_of_neighbouring_vertives.erase (itr);
								keep_adding = true;
								break;
							}
						}

					}
				}


			#if MODE_DEBUG
				out_stream << "RING: ";
				for (auto val : ring)
					out_stream << std::setw (5) << val << ' ';
				out_stream << '\n';
			#endif

				auto angle_between_edges = [&](uint32_t curr_vertex_index, uint32_t other1_vertex_index, uint32_t other2_vertex_index) -> float {
					glm::vec3 edge1 = glm::normalize (posn_and_normals[other1_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					glm::vec3 edge2 = glm::normalize (posn_and_normals[other2_vertex_index].first - posn_and_normals[curr_vertex_index].first);
					float e1e2sin_theta = glm::length (glm::cross (edge1, edge2));
					return asinf (MIN (1.0, e1e2sin_theta));
				};
				auto cotf = [](float angle_in_rad) -> float { return (angle_in_rad < glm::half_pi<float> ()) ? tanf (glm::half_pi<float> () - angle_in_rad) : -tanf (angle_in_rad - glm::half_pi<float> ()); };
				auto sqr = [](float val) -> float { return val*val; };
				//      /\X
				//     /  \
				//   Q/____\R
				float A_mixed = 0;
				glm::vec3 sigma_mean_curvature_normal_operator = glm::vec3 (0);
				for (uint32_t i = 1; i < ring.size (); i++) {
					float Q = angle_between_edges (ring[i-1], ring[i], curr_indice)
						, R = angle_between_edges (ring[i], curr_indice, ring[i-1])
						, X = angle_between_edges (curr_indice, ring[i], ring[i-1]);
					float pXR2, pXQ2, cot_Q, cot_R;
					glm::vec3 XR_diff, XQ_diff;
					{
						XR_diff = posn_and_normals[curr_indice].first - posn_and_normals[ring[i-1]].first;
						XQ_diff = posn_and_normals[curr_indice].first - posn_and_normals[ring[i]].first;
						cot_Q = cotf (Q), cot_R = cotf (R);
						pXR2 = glm::length2 (XR_diff), pXQ2 = glm::length2 (XQ_diff), cot_Q, cot_R;
					}

					if (Q < glm::half_pi<float> () && R < glm::half_pi<float> () && X < glm::half_pi<float> ()) { // Acute △
						float A_voronoi = (pXR2*cot_Q + pXQ2*cot_R)*0.125; // (1/8)*((X-R)^2 * cotf(∠Q)  +  (X-Q)^2 * cotf(∠R))
						A_mixed += A_voronoi;
					} else {
						float Area_T = (pXR2*cot_Q/sqr (sinf (R)) + pXQ2*cot_R/sqr (sinf (Q)))*0.5;
						if (X >= glm::half_pi<float> ())
							A_mixed += Area_T*0.5;
						else
							A_mixed += Area_T*0.25;
					}

					glm::vec3 mean_curvature_normal_operator_for_curr_tri
						= cot_Q*XR_diff +  cot_R*XQ_diff;
					sigma_mean_curvature_normal_operator += mean_curvature_normal_operator_for_curr_tri;
				}
				K_Xi = (sigma_mean_curvature_normal_operator)*float (1.0/(2.0*A_mixed));

				if (trackOutput) {
					cout_stream << std::setw (4) << curr_indice << ' ' << A_mixed << ' ';
				}
			#if MODE_DEBUG
				out_stream << "A_mixed: " << A_mixed;
			#endif
			}
			float K_h = glm::length (K_Xi)*0.5;

			mutex_curvature_push.lock ();
			array_K_Xi[curr_indice] = K_Xi;
			array_K_h[curr_indice] = K_h;
			max_curvature = MAX (K_h, max_curvature);
			min_curvature = MIN (K_h, min_curvature);
			mutex_curvature_push.unlock ();
			if (trackOutput)
				cout_stream << K_h << ' ' << K_Xi << '\n';

		#if MODE_DEBUG
			out_stream << " mean_curvature: " << K_h << " K(Xi) " << K_Xi << "\ncurrvertex: " << posn_and_normals[curr_indice].first << " normal: " << posn_and_normals[curr_indice].second << '\n';
		#endif
		}
		{ // brain blown away
			barrier.notify_all ();
			another_sync_lock.lock ();
			if (vertices_processed < posn_and_normals.size ()) { // not changing the owner thread, other threads skips it
				barrier.wait (synced_lock, [&] { return vertices_processed >= posn_and_normals.size (); });
				if (!trackOutput)
					std::cout << "\r  vertices_processed: " << vertices_processed << " out_of: " << posn_and_normals.size () << '\n';
				else
					std::cout << "\nresulting mean_curvatures{max: " << max_curvature << ", min: " << min_curvature << "}\n\n";
				synced_lock.unlock ();
			}
			std::cout << cout_stream.str ();
			another_sync_lock.unlock ();
		}

		float min_max_curvature_diff = (max_curvature - min_curvature);
	#if MODE_DEBUG

		mutex_ofs.lock ();
		if (first_to_reach_access_it) {
			ofs << "mean_curvatures{max: " << max_curvature << ", min: " << min_curvature << "}\n\n";
			first_to_reach_access_it = false;
		}
		ofs << out_stream.str ();
		mutex_ofs.unlock ();
	#endif

		//for (size_t i = start; i < array_K_Xi.size (); i += stride) {
		//	curvature_diffuse_color[i] = -glm::normalize(array_K_Xi[i]);
		//}
		auto blend = [](float ratio, const std::vector<glm::vec3> &blend_between) -> glm::vec3 {
			ratio *= (blend_between.size () - 1);
			float low_contri = std::floor (ratio);
			float high_contri = std::ceil (ratio);
			int low = low_contri;
			int high = high_contri;
			return low_contri*blend_between[low] + high_contri*blend_between[high];
		};
		for (size_t i = start; i < array_K_Xi.size (); i += stride) {
			float ratio = array_K_h[i];
			ratio -= min_curvature;
			ratio /= min_max_curvature_diff;

			curvature_diffuse_color[i] = blend (ratio, blend_betweencolors);
		};
	};
#define USE_MT 1
#if USE_MT
	std::vector<std::thread> threads;
	threads.reserve (std::thread::hardware_concurrency () - 1);
	for (uint32_t i = 1; i < std::thread::hardware_concurrency (); i++)
		threads.push_back (std::thread (mean_curvature_func, i, std::thread::hardware_concurrency ()));
	mean_curvature_func (0, std::thread::hardware_concurrency ());
	for (std::thread &ref : threads)
		ref.join ();
#else
	mean_curvature_func (0, 1);
#endif
#if MODE_DEBUG
	ofs.close ();
#endif
	mean_curvature_normals = std::move (array_K_Xi), mean_curvature_values = std::move (array_K_h);
	if (save_min_mean_curvature)
		*save_min_mean_curvature = min_curvature;
	if (save_min_mean_curvature)
		*save_max_mean_curvature = max_curvature;
	return true;
}