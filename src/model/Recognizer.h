#ifndef RECOGNIZER_H_
#define RECOGNIZER_H_

#include <Model.h>
#include <Word.h>
#include <vector>

using namespace std;
using namespace yazz::audio;

namespace yazz {
namespace math {

/**
 * Recognizer tries to determine which model suits better for the specific word
 */
class Recognizer {
public:

	Recognizer(vector<Model*>* models);
	~Recognizer();

	const Model* recognize(const Word& word);

private:
	const vector<Model*>* models;
};

} /* namespace math */
} /* namespace yazz */

#endif /* RECOGNIZER_H_ */
