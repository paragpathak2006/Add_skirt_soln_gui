#include "..\HeaderFiles\myApp.h"

void myApp::processInput(GLFWwindow* win, double elapsed)
{
	Camera* cam = graph.getCam();

	if (reset_cam) {
		cam->eye = Vector3D(0, 0, 20);
		cam->center = Vector3D(0, 0, 0);
		cam->up = Vector3D(0, 1, 0);

		cam->updateView();
		reset_cam = false;
	}
	else {
		if (first_person) {
			walk(win, elapsed);
			fps_look(win, elapsed);
		}
		else if (move_camera) {
			camera_movement(win, elapsed);
		}
		obj_arrows(win, elapsed);
	}

	if (gizmoActive) {
		manipulateGizmo(win, elapsed);
	}

	animate(win, elapsed);
}

void myApp::manipulateGizmo(GLFWwindow* win, double elapsed)
{
	double x, y;
	glfwGetCursorPos(win, &x, &y);

	std::string name = graph.getSelected()->name;
	TransformInfo info;
	GizmoType type = graph.getGizmoType();
	Vector3D world3D = worldDir.to3D();

	if (type == GizmoType::Translation) {
		Vector2D currDir = Vector2D(x - gizmo_x, gizmo_y - y);

		double dot = currDir * screenDir;

		info = MxFactory::translate(world3D * dot * 0.1);
	}
	else if (type == GizmoType::Scaling) {
		Vector2D currDir = Vector2D(x - gizmo_x, y - gizmo_y);

		info = MxFactory::scale((Vector3D(1, 1, 1) - world3D) + (world3D * fabs(currDir.length()) * scale));
	}
	else if (type == GizmoType::Rotation) {
		Vector2D currDir = Vector2D(x - gizmo_x, y - gizmo_y).normalize();

		double cos = currDir * screenDir;
		if (cos > 1) cos = 1;
		if (cos < -1) cos = -1;
		
		Vector3D axis = screenDir.to3D() % currDir.to3D();
		if (axis.length() != 0) {
			axis.normalize();
		}

		double angleDeg = scale * -axis.z * acos(cos) / M_PI_2 * 90;

		info = MxFactory::rotate(world3D, angleDeg);
	}

	TransformInfo parent = graph.getSelected()->parent->getTransformInfo();
	TransformInfo translate = MxFactory::translate(pos);

	graph.setTransforms(name, { localTransform, parent, translate.invert(), info, translate, parent.invert() });
}

void myApp::animate(GLFWwindow* win, double elapsed)
{
	/** /
	t_frame += elapsed;

	graph.setTransforms("cube", { MxFactory::scale(Vector3D(2, 1, 1)), MxFactory::rotate(Vector3D(0,1,0), t_frame * 90) });
	/**/
}

void myApp::walk(GLFWwindow* win, double elapsed)
{
	Camera* cam = graph.getCam();
	int r = (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS),
		l = (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS),
		d = (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS),
		u = (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);

	double xaxis = (double)r - l,
		yaxis = (double)d - u;

	if (xaxis != 0 || yaxis != 0) {
		Vector3D dir = Vector3D(xaxis, 0, -yaxis);
		dir.normalize();

		cam->move(dir, 10 * elapsed);
	}
}

void myApp::fps_look(GLFWwindow* win, double elapsed) {
	Camera* cam = graph.getCam();

	double x, y;
	glfwGetCursorPos(win, &x, &y);

	double move_x = (x - old_x);
	double move_y = (y - old_y);

	double percent_x = fabs(M_PI_2 * move_x) / w;
	double percent_y = fabs(M_PI_2 * move_y) / h;

	if (move_x != 0 || move_y != 0)
		cam->fpslook(percent_x * move_x * elapsed, percent_y * move_y * elapsed);

	old_x = x;
	old_y = y;
}

void myApp::camera_movement(GLFWwindow* win, double elapsed)
{
	Camera* cam = graph.getCam();

	double x, y;
	glfwGetCursorPos(win, &x, &y);

	double move_x = (x - old_x);
	double move_y = (y - old_y);

	double x_percent = fabs(move_x) / (double) w;
	double y_percent = fabs(move_y) / (double) y;

	if (move_x != 0 || move_y != 0)
		if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			Vector3D dir = Vector3D(x_percent * -move_x, y_percent * move_y, 0);
			
			cam->move(dir, 1);
		}
		else {
			cam->look(- move_x * 0.5, -move_y * 0.5);
		}

	old_x = x;
	old_y = y;
}

void myApp::save(GLFWwindow* win)
{
	int w, h;
	glfwGetWindowSize(win, &w, &h);
	
	GLubyte* pixels = new GLubyte[3 * static_cast<size_t>(w * h)];

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, pixels);

	// Convert to FreeImage format & save to file
	FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, w, h, 3 * w, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);
	FreeImage_Save(FIF_PNG, image, "test.png", 0);

	// Free resources
	FreeImage_Unload(image);
	delete[] pixels;
}

void myApp::populateScene()
{
	// Camera init
	Camera* cam = new Camera(Vector3D(8, 8, 8), Vector3D(0, 0, 0), Vector3D(0, 1, 0));
	cam->perspectiveProjection(60, 16.0f / 9.0f, 1, 200);
	cam->init();

	graph.setCamera(cam);
	
	// Outline
	Shader* outline_shader = new Shader("outline", "res/shaders/outline_vs.glsl", "res/shaders/outline_fs.glsl");
	outline_shader->addUniformBlock("Matrices", 0);
	Material* outline_material = new Material(outline_shader);
	graph.setOutline(outline_material);

	//Asset Manager
	Manager* h = Manager::getInstance();

	// Meshes
	Mesh* cube_mesh = h->addMesh("cube_mesh", new Mesh("cube_mesh", "res/meshes/bunny_smooth.obj"));
	Mesh* torus_mesh = h->addMesh("torus_mesh", new Mesh("torus_mesh", "res/meshes/torus.obj"));

	// Shaders
	Shader* blinn_phong_shader = h->addShader("blinn_phong_shader", new Shader("blinn_phong_shader", "res/shaders/blinn_phong_vs.glsl", "res/shaders/blinn_phong_fs.glsl"));
	blinn_phong_shader->addUniformBlock("Matrices", 0);
	Shader* gooch_shader = h->addShader("gooch_shader", new Shader("gooch_shader", "res/shaders/gooch_vs.glsl", "res/shaders/gooch_fs.glsl"));
	gooch_shader->addUniformBlock("Matrices", 0);

	// Materials 
	Material* blinnphong_mat = h->addMaterial("blinnphong_mat", new Material(blinn_phong_shader));
	blinnphong_mat->setUniformVec3("u_AlbedoColor", Vector3D(0.8f, 0.8f, 0.8f));
	Material* gooch_mat = h->addMaterial("gooch_mat", new Material(gooch_shader));
	gooch_mat->setUniformVec3("u_AlbedoColor", Vector3D(0.8f, 0.8f, 0.8f));

	//cube
	graph.addChild(gooch_mat, cube_mesh, "cube");
	graph.getNode("cube")->meshName = "cube_mesh";
	graph.getNode("cube")->meshFile = "res/meshes/bunny_smooth.obj";
	graph.getNode("cube")->materialName = "gooch_mat";
	graph.getNode("cube")->shaderName = "blinn_phong_shader";
	graph.getNode("cube")->vertexShaderFile = "res/shaders/blinn_phong_vs.glsl";
	graph.getNode("cube")->fragmentShaderFile = "res/shaders/blinn_phong_fs.glsl";

	//torus
	graph.addChild(blinnphong_mat, torus_mesh, "torus", MxFactory::translation4(Vector3D(0,0,-5)));
	graph.getNode("torus")->meshName = "torus_mesh";
	graph.getNode("torus")->meshFile = "res/meshes/torus.obj";
	graph.getNode("torus")->materialName = "blinnphong_mat";
	graph.getNode("torus")->shaderName = "blinn_phong_shader";
	graph.getNode("torus")->vertexShaderFile = "res/shaders/blinn_phong_vs.glsl";
	graph.getNode("torus")->fragmentShaderFile = "res/shaders/blinn_phong_fs.glsl";
	graph.setTransforms("torus", { MxFactory::translate(Vector3D(0,0,-5)) });

	/* ----------------------------------------------------------------------------------------------------- */

	//Grid
	Mesh* plane_mesh = h->addMesh("plane_mesh", new Mesh("plane_mesh", "res/meshes/plane.obj"));
	Shader* grid_shader = h->addShader("grid_shader", new Shader("grid_shader", "res/shaders/grid_vs.glsl", "res/shaders/grid_fs.glsl"));
	grid_shader->addUniformBlock("Matrices", 0);
	Material* grid_mat = h->addMaterial("grid_mat", new Material(grid_shader));
	graph.setGrid(grid_mat, plane_mesh, MxFactory::scale(Vector3D(100, 100, 100)));

	//HUD
	Mesh* hud_mesh = h->addMesh("hud_mesh", new Mesh("hud_mesh", "res/meshes/hud.obj"));
	Texture* hud_texture = h->addTexture("hud_texture", new Texture("hud_texture", "res/textures/hud_texture.psd"));
	Shader* hud_shader = h->addShader("hud_shader", new Shader("hud_shader", "res/shaders/hud_vs.glsl", "res/shaders/hud_fs.glsl"));
	Material* hud_mat = h->addMaterial("hud_mat", new Material(hud_shader));
	hud_mat->setTexture(hud_texture);
	graph.setHud(hud_mat, hud_mesh);

	//Gizmos
	h->addMesh("translation_gizmo", new Mesh("translation_gizmo", "res/meshes/Gizmos/TranslationGizmo.obj"));
	h->addMesh("scale_gizmo", new Mesh("scale_gizmo", "res/meshes/Gizmos/ScaleGizmo.obj"));
	h->addMesh("rotation_gizmo", new Mesh("rotation_gizmo", "res/meshes/Gizmos/AltRotationGizmo.obj"));

	Shader* gizmo_shader = h->addShader("gizmo_shader", new Shader("gizmo_shader", "res/shaders/gizmo_vs.glsl", "res/shaders/gizmo_fs.glsl"));
	gizmo_shader->addUniformBlock("Matrices", 0);
}


void cleanNode(SceneGraph graph, std::vector<SceneNode*> nodes) {
	for (auto node : nodes) {
		if (node->children.size() != 0) {
			cleanNode(graph, node->children);
		}
		graph.removeObject(node->name);
	}
}

void myApp::cleanScene() {
	SceneNode* root = graph.getNode("root");

	for (auto node : root->children) {
		if (node->children.size() != 0) {
			cleanNode(graph, node->children);
		}
		graph.removeObject(node->name);
	}
}

void myApp::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
	Camera* cam = graph.getCam();
	if (action == GLFW_RELEASE) {
		switch (key) {
		case GLFW_KEY_P:
			if (cam->projType == CameraProj::Parallel) {
				cam->perspectiveProjection(60, 16.0f / 9.0f, 1, 50);
			}
			else {
				cam->parallelProjection(-10, 10, -10, 10, 1, 50);
			}
			break;
		case GLFW_KEY_F:
			animate_frame = true;
			break;
		case GLFW_KEY_R:
			if (!gizmoActive)
			graph.setGizmoType(GizmoType::Rotation);
			break;
		case GLFW_KEY_E:
			if (!gizmoActive)
			graph.setGizmoType(GizmoType::Scaling);
			break;
		case GLFW_KEY_G:
			if (!gizmoActive)
			graph.setGizmoType(GizmoType::Translation);
			break;
		case GLFW_KEY_I:
			save_img = true;
			break;
		case GLFW_KEY_M:
			choosing_object = !choosing_object;
			break;
		case GLFW_KEY_DELETE:
			if (graph.getSelected()) {
				graph.removeObject(graph.getSelected()->name);
			}
			break;
		case GLFW_KEY_1:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 1;
			}
			break;
		case GLFW_KEY_2:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 2;
			}
			break;
		case GLFW_KEY_3:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 3;
			}
			break;
		case GLFW_KEY_4:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 4;
			}
			break;
		case GLFW_KEY_5:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 5;
			}
			break;
		case GLFW_KEY_6:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 6;
			}
			break;
		case GLFW_KEY_7:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 7;
			}
			break;
		case GLFW_KEY_8:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 8;
			}
			break;
		case GLFW_KEY_9:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 9;
			}
			break;
		case GLFW_KEY_0:
			if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 10;
			}
			break;
		case GLFW_KEY_N:
			/*if (choosing_object)
			{
				add_mesh = true;
				mesh_indicator = 11;
			}
			else*/
			new_mat = true;
			break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_ENTER: { //save a scene
			saveScene("scene");
			break;
		}
		case GLFW_KEY_L: //load a saved scene
			loadScene("scene");
			break;
		}
	}
}

void myApp::mouseCallback(GLFWwindow* win, double xpos, double ypos) {
	
}

void myApp::scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
	Camera* cam = graph.getCam();

	cam->zoom(yoffset);
}

void myApp::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			first_person = true;

			double x, y;
			glfwGetCursorPos(win, &x, &y);
			glfwGetWindowSize(win, &w, &h);

			old_x = x; old_y = y;
		}
		else if (action == GLFW_RELEASE) {
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			first_person = false;
		}
	}
	
	if (first_person) return;

	if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
		if (action == GLFW_PRESS) {
			move_camera = true;
			
			double x, y;
			glfwGetCursorPos(win, &x, &y);
			glfwGetWindowSize(win, &w, &h);

			old_x = x; old_y = y;
		}
		else if (action == GLFW_RELEASE) {
			move_camera = false;
		}
	}

	if (move_camera) return;

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		double xpos, ypos;
		int height, width;
		glfwGetCursorPos(win, &xpos, &ypos);
		int x = static_cast<int>(xpos);

		glfwGetWindowSize(win, &width, &height);
		int y = height - static_cast<int>(ypos);

		GLuint index;
		glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

		if (action == GLFW_PRESS) {
			if (index >= 0xFD) {
				gizmoActive = true;
				graph.gizmoActive = true;

				if (index == 0xFF) {
					worldDir = Vector4D(1, 0, 0, 1);
				}
				else if (index == 0xFE) {
					worldDir = Vector4D(0, 1, 0, 1);
				}
				else if (index == 0xFD) {
					worldDir = Vector4D(0, 0, 1, 1);
				}

				Camera* cam = graph.getCam();

				Matrix4 projView = cam->projection * cam->view;

				fullTransform = graph.getSelected()->getTransformInfo();
				localTransform = { graph.getSelected()->transform, graph.getSelected()->inverse };

				Vector4D center = projView * fullTransform.transform * Vector4D(0,0,0,1);
				center.divideByW();	
			
				Vector4D point;
				
				GizmoType gizType = graph.getGizmoType();

				Vector4D pos4 = fullTransform.transform * Vector4D(0, 0, 0, 1);
				pos = pos4.to3D();

				if (gizType == GizmoType::Translation) {
					point = projView * (pos4 + worldDir);
					point.divideByW();

					gizmo_x = xpos;
					gizmo_y = ypos;

					screenDir = point.to2D() - center.to2D();
				}
				else if (gizType == GizmoType::Scaling) {
					gizmo_x = (center.x + 1) / 2 * width;
					gizmo_y = height - (center.y + 1) / 2 * height;

					Vector2D currDir = Vector2D(xpos - gizmo_x, ypos - gizmo_y);

					scale = 1 / currDir.length();
				}
				
				else if (gizType == GizmoType::Rotation) {
					gizmo_x = (center.x + 1) / 2 * width;
					gizmo_y = height - (center.y + 1) / 2 * height;

					Vector3D relativeEye = cam->eye - cam->center;

					scale = relativeEye * worldDir.to3D();

					if (scale == 0) scale = 1;
					else scale /= fabs(scale);

					screenDir = Vector2D(xpos - gizmo_x, ypos - gizmo_y).normalize();
				}
			}
		}
		if (action == GLFW_RELEASE) {
			if (gizmoActive) {
				gizmoActive = false;
				graph.gizmoActive = false;
			}
			else if (index < 0xFD) {
				if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
					graph.changeParent(index);
				}
				else {
					graph.setSelected(index);
				}
			}
		}
	}
}

void myApp::obj_arrows(GLFWwindow* win, double elapsed)
{
	int r = (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS),
		l = (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS),
		d = (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS),
		u = (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);

	double move_v = (double)u - d,
		move_h = (double)r - l;

	if (move_v == 0 && move_h == 0) return;

	SceneNode* selected = graph.getSelected();
	if (!selected) return;

	Vector4D pos = selected->getTransformInfo().transform * Vector4D(0, 0, 0, 1);
	TransformInfo parent = selected->parent->getTransformInfo(),
		translate = MxFactory::translate(pos.to3D()),
		localTransform = { selected->transform, selected->inverse },
		extraTransform;

	Camera* cam = graph.getCam();

	if (graph.getGizmoType() == GizmoType::Translation) {
		Vector3D translationDir = cam->u * move_v + cam->s * move_h;
		translationDir.normalize();
		extraTransform = MxFactory::translate(3 * translationDir * elapsed);
	} 
	else if (graph.getGizmoType() == GizmoType::Scaling) {
		Vector3D scaleDir = cam->u * move_v + cam->s * move_h;
		scaleDir.normalize();
		extraTransform = MxFactory::scale(Vector3D(1, 1, 1) + (scaleDir * elapsed));
	}
	else if (graph.getGizmoType() == GizmoType::Rotation) {
		Vector3D rotateDir = cam->s * -move_v + cam->v * move_h;
		rotateDir.normalize();
		extraTransform = MxFactory::rotate(rotateDir, 1);
	}

	graph.setTransforms(selected->name, 
		{ localTransform, parent, translate.invert(), extraTransform, translate, parent.invert() });

}


void myApp::update(GLFWwindow *win, double elapsed)
{
	processInput(win, elapsed);

	graph.draw();
	if (save_img) {
		save(win);
		save_img = false;
	}
	if (new_mat) {
		Manager* h = Manager::getInstance();
		SceneNode* node = graph.getNode("cube");

		Shader* toon_shader = h->addShader("toon_shader", new Shader("toon_shader", "res/shaders/toon_vs.glsl", "res/shaders/toon_fs.glsl"));
		toon_shader->addUniformBlock("Matrices", 0);

		Texture* toon_ramp_texture = h->addTexture("toon_ramp_texture", new Texture("toon_ramp_texture", "res/textures/toon_ramp_texture.png"));

		Material* test_mat_b = h->addMaterial("test_mat_b", new Material(toon_shader));
		test_mat_b->setUniformVec3("u_AlbedoColor", Vector3D(0, 0, 1));
		test_mat_b->setTexture(toon_ramp_texture);

		node->material = test_mat_b;
		node->textureName = "toon_ramp_texture";
		node->textureFile = "res/textures/toon_ramp_texture.png";
		new_mat = false;
	}
	if (add_mesh && mesh_indicator ==1) { 
		loadObject("cube");
	}
	if (add_mesh && mesh_indicator == 2) {
		loadObject("cylinder");
	}
	if (add_mesh && mesh_indicator == 3) { 
		loadObject("frame");
	}
	if (add_mesh && mesh_indicator == 4) { 
		loadObject("plane");
	}
	if (add_mesh && mesh_indicator == 5) { 
		loadObject("hand");
	}
	if (add_mesh && mesh_indicator == 6) { 
		loadObject("teapot");
	}
	if (add_mesh && mesh_indicator == 7) { 
		loadObject("torus");
	}
	if (add_mesh && mesh_indicator == 8) { 
		loadObject("backpiece");
	}
	if (add_mesh && mesh_indicator == 9) { 
		loadObject("suzanne");
	}
	if (add_mesh && mesh_indicator == 10) { 
		loadObject("bunny");
	}
	/*if (add_mesh && mesh_indicator == 11) {
		
		std::string meshname;
		cout << "Please enter the name of your .obj file after the '>'\n >";
		cin >> meshname;
		cout << meshname + "should be loading now :^)\n"; 
		
		loadObject(meshname);
		graph.describe(); //debug
	}*/
}

void myApp::processCommands(queue<std::string> &commands)
{
	std::string command;
	while (!commands.empty()) 
	{
		command = commands.front();
		this->executeCommand(command);
		commands.pop();

		std::cout << "Enter command:";
	}
}

void myApp::executeCommand(std::string command)
{
	//init components
	vector <string> tokens;
	stringstream parsable(command);
	string current;
	size_t pos = 0;


	// tokenise
	while ( getline(parsable, current, ',' )) {
		tokens.push_back(current);
	}

	//print to terminal - debug
	//for (int i = 0; i < tokens.size(); i++) {
	//	cout << tokens[i] << '\n';
	//}
	
	if (tokens[0] == "LoadObject") {
		loadObject(tokens[1]);
	}
	else if (tokens[0] == "ImportMesh") { 
		importMesh(tokens[1]);
	}
	else if (tokens[0] == "ImportShader") {
		importShader(tokens[1]);
	}
	else if (tokens[0] == "ImportTexture") {
		importTexture(tokens[1], tokens[2]);
	}
	else if (tokens[0] == "CreateMaterial") {
		createMaterial(tokens[1], tokens[2]);
	}
	else if (tokens[0] == "CreateObject") {
		createObject(tokens[1], tokens[2], tokens[3]);
	}
	else if (tokens[0] == "RemoveObject") {
		removeObject(tokens[1]);
	}
	else if (tokens[0] == "DescribeScene") {
		graph.describe();
	}
	else if (tokens[0] == "ObjectSetMaterial") {
		objectSetMaterial(tokens[1], tokens[2]);
	}
	else if (tokens[0] == "MaterialSetUniform") {
		materialSetUniform(tokens[1], tokens[2], tokens[3], tokens[4]);
	}
	else if (tokens[0] == "ObjectSetParent") {
		objectSetParent(tokens[1], tokens[2]);
	}
	else if (tokens[0] == "LoadScene") {
		loadScene(tokens[1]);
	}
	else if (tokens[0] == "SaveScene") {
		saveScene(tokens[1]);
	}
	else if (tokens[0] == "SeeAssets") {
		seeAssets();
	}

	else if (tokens[0] == "Help") {
		cout << "--------------------------------------------------------------------------\n"
			 << "| Here is a list of our commands in the format they should be written:   |\n"
			 << "|                                                                        |\n"
			 << "|                   -----Import instructions-----                        |\n"
			 << "| * ImportMesh,meshname                                                  |\n" 
			 << "| * ImportShader,shadername                                              |\n"
			 << "| * ImportTexture,texturename,format                                     |\n"
			 << "|                                                                        |\n"
			 << "|                         -----Objects-----                              |\n"
			 << "| * LoadObject,objectname                                                |\n"
			 << "| * CreateObject,objectname,meshname,materialname                        |\n"
			 << "| * RemoveObject,objectname                                              |\n"
			 << "| * ObjectSetParent,objectname,parentname                                |\n"
			 << "|                                                                        |\n"
			 << "|                        -----Materials-----                             |\n"
			 << "| * CreateMaterial,materialname,shadername                               |\n"
			 << "| * ObjectSetMaterial,objectname,materialname                            |\n"
			 << "| * MaterialSetUniform,materialname,uniformname,uniformtype,uniformvalue |\n"
			 << "|                                                                        |\n"
			 << "|                          -----Scene-----                               |\n"
	   		 << "| * SeeAssets                                                            |\n"
			 << "| * DescribeScene                                                        |\n"
			 << "| * SaveScene,savename                                                   |\n"
			 << "| * LoadScene,savename                                                   |\n"
			 << "|                                                                        |\n" 
			 << "| For more detailed information please refer to the manual               |\n"
			 << "-------------------------------------------------------------------------\n";
	}
	else
		cout << "Sorry but that command does not exist!\n";
	
}

bool myApp::checkFile(string filename) {
	FILE* file;
	const char* filenameChar = filename.c_str();
	if (errno_t err = fopen_s(&file, filenameChar, "r") != 0) {
		cout << "No such file exists\n";
		return false;
	}
	else {
		true;
	}

}

bool myApp::importMesh(string meshname) {
	Manager* h = Manager::getInstance();
	string meshlocation = "res/meshes/" + meshname + ".obj";
	if (checkFile(meshlocation)) {
		Mesh* mesh = h->addMesh(meshname, new Mesh(meshname, meshlocation));
		return true;
	}
	return false;
}

bool myApp::importShader(string shadername) {
	Manager* h = Manager::getInstance();
	string shaderFragment = "res/shaders/" + shadername + "_fs.glsl";
	string shaderVertex = "res/shaders/" + shadername + "_vs.glsl";
	if (checkFile(shaderFragment) && checkFile(shaderVertex)) {
		Shader* shader = h->addShader(shadername, new Shader(shadername, shaderVertex, shaderFragment));
		shader->addUniformBlock("Matrices", 0);
		return true;
	}
	return false;
}

bool myApp::importTexture(string texturename, string format) {
	Manager* h = Manager::getInstance();
	string texturelocation = "res/textures/" + texturename + "." + format;
	if (checkFile(texturelocation)) {
		Texture* texture = h->addTexture(texturename, new Texture(texturename, texturelocation));
		return true;
	}
	return false;
}

void myApp::loadObject(string objecttype) {
	Manager* h = Manager::getInstance();
	std::string id = std::to_string(h->getCounter());
	string meshname = objecttype;
	string objectname = objecttype + id;
	if (!h->hasMesh(meshname)) {
		if (importMesh(meshname)) {
			Mesh* mesh = h->getMesh(meshname);
			mesh->init();
			Material* material = h->getMaterial("blinnphong_mat");

			graph.setCurrToRoot();
			graph.addChild(material, mesh, objectname);
			graph.setTransforms(objectname, { MxFactory::translate(Vector3D(1,1,1)) });
		}
	}
	add_mesh = false;
	mesh_indicator = 0;
	choosing_object = false;
}

void myApp::createObject(string objecttype, string meshname, string materialname) {
	Manager* h = Manager::getInstance();
	std::string id = std::to_string(h->getCounter());
	string objectname = objecttype + id;
	if (!h->hasMesh(meshname))
		if (importMesh(meshname)) {
			Mesh* mesh = h->getMesh(meshname);
			mesh->init();
			Material* material = h->getMaterial(materialname);

			graph.setCurrToRoot();
			graph.addChild(material, mesh, objectname);
			graph.setTransforms(objectname, { MxFactory::translate(Vector3D(1,1,1)) });
		}
	add_mesh = false;
	mesh_indicator = 0;
}

void::myApp::removeObject(string objname) {
	graph.removeObject(objname);
}

void myApp::createMaterial(string materialname, string shadername) {
	Manager* h = Manager::getInstance();
	Shader* shader = h->getShader(shadername);
	Material* material = new Material(shader); 
	h->addMaterial(materialname, material);
}

void myApp::objectSetMaterial(string objname, string materialname) {
	Manager* h = Manager::getInstance();
	Material* material = h->getMaterial(materialname);
	graph.changeMaterial(objname, material);

}

void myApp::materialSetUniform(string materialname, string uniformname, string uniformtype, string uniformvalue) {
	Manager* h = Manager::getInstance();
	Material* material = h->getMaterial(materialname);
	
	if (uniformtype == "float") {
		material->setUniform1float(uniformname, std::stof(uniformvalue));
	}
	else if (uniformtype == "int") {
		material->setUniform1int(uniformname, std::stoi(uniformvalue));
	}
	else if (uniformtype == "texture") {
		Texture * tex = h->getTexture(uniformvalue);
		material->setTexture(tex);
	}
}

void myApp::objectSetParent(string objname, string parentname) {
	graph.changeParent(objname, parentname);
}

void myApp::seeAssets() {
	Manager* h = Manager::getInstance();
	std::unordered_map<std::string, Shader*> shaders = h->getShaders();
	std::unordered_map<std::string, Mesh*> meshes = h->getMeshes();
	std::unordered_map<std::string, Texture*> textures = h->getTextures();
	std::unordered_map<std::string, Material*> materials = h->getMaterials();
	cout << "\nShaders:\n";
	for (auto i : shaders) {
		cout << " - " + i.first + "\n";
	}
	cout << "Meshes:\n";
	for (auto i : meshes) {
		cout << " - " + i.first + "\n";
	}
	cout << "Textures:\n";
	for (auto i : textures) {
		cout << " - " + i.first + "\n";
	}
	cout << "\Materials:\n";
	for (auto i : materials) {
		cout << " - " + i.first + "\n";
	}
}

void myApp::saveScene(string savename) {
	Manager* h = Manager::getInstance();
	Camera* cam = graph.getCam();
	const std::string path = "res/scenes/" + savename + ".txt";
	graph.serializeScene(cam, h, path);
	cout << "Scene saved.\n";
}

void myApp::loadScene(string savename) {
	const std::string path = "res/scenes/" + savename + ".txt";

	//Clean current scene
	cleanScene();
	Manager::getInstance()->destroy();
	graph.loadScene(path);

	Manager* h = Manager::getInstance();
	cout << h->getShaders().size();
}