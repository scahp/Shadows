#version 330 core

#preprocessor

in float ClipDepth;
in float Depth;

out float Result;

void main()
{
	if (ClipDepth < 0.0)
		discard;

	Result = Depth;
}

