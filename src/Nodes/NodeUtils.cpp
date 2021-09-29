#include <NodeEditorNodes.h>

int GenerateId() {
	static int id = 1;
	int retVal = id;
	id = id + 1;
	return retVal;
}