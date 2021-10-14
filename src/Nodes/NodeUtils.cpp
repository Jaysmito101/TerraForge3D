#include <NodeEditorNodes.h>

static int id = 1;

int GenerateId() {
	int retVal = id;
	id = id + 1;
	return retVal;
}

void SetBaseId(int baseID)
{
	id = baseID;
}
