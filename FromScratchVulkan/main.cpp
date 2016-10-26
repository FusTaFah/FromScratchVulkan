#include "Renderer.h"

int main() {
	Renderer r;
	r.CreateVulkanWindow(800, 600, "test");
	while (r.Run()) {
		
	}
	return 0;
}