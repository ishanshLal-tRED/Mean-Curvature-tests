
bool MeanCurvatureCalculate (const char *debug_filename
							 , const std::vector<std::pair<glm::vec3, glm::vec3>> &posn_and_normals, const std::vector<GLuint> &indices, std::vector<glm::vec3> &curvature_diffuse_color
							 , std::vector<glm::vec3> &mean_curvature_normals, std::vector<float> &mean_curvature_values
							 , const std::vector<glm::vec3> &blend_betweencolors
							 , const bool trackOutput = true, float *save_min_mean_curvature = nullptr, float *save_max_mean_curvature = nullptr);