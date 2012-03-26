int main() {
	try {
		throw 4;
	}
	catch(int x) {
		x++;
	}
	return 0;
}

