#define NS_ERROR_DOM_DOMSTRING_SIZE_ERR -1
#define NS_ERROR_DOM_INDEX_SIZE_ERR 	-2
#define NS_ERROR_OUT_OF_MEMORY 		-3
#define NS_ENSURE_TRUE(c,r) 		if (!c) return (r)

typedef int PRInt32;
typedef unsigned int PRUint32;
typedef char PRUnichar;
typedef int PRBool;
typedef int nsresult;
typedef struct {} nsIDocument;
typedef void * nsCOMPtr;

extern void * memcpy(...);
extern nsIDocument * GetCurrentDoc(void);
extern void *GetCurrentValueAtom(void);
extern void nsNodeUtils_CharacterDataWillChange(void);
extern void SetTo(...);
extern void Append(...);
extern void CopyTo(...);

nsresult
nsGenericDOMDataNode_SetTextInternal(PRUint32 aOffset, PRUint32 aCount,
		const PRUnichar* aBuffer,
		PRUint32 aLength, PRBool aNotify)
{
	int r = 0;
	// sanitize arguments
	PRUint32 textLength;
	if (aOffset > textLength) {
		r = NS_ERROR_DOM_INDEX_SIZE_ERR;
		return r;
	}

	if (aCount > textLength - aOffset) {
		aCount = textLength - aOffset;
	}

	PRUint32 endOffset = aOffset + aCount;

	// Make sure the text fragment can hold the new data.
	PRInt32 newLength = textLength - aCount + aLength ;
	if (aLength > aCount && (newLength > 536870912 || (-3758096384 < newLength && newLength < 0))) {
		// This exception isn't per spec, but the spec doesn't actually
		// say what to do here.
		r = NS_ERROR_DOM_DOMSTRING_SIZE_ERR;
		return r;
	}

	nsIDocument *document = GetCurrentDoc();

	PRBool haveMutationListeners;

	nsCOMPtr oldValue;
	if (haveMutationListeners) {
		oldValue = GetCurrentValueAtom();
	}

	if (aNotify) {
		nsNodeUtils_CharacterDataWillChange();
	}

	if (aOffset == 0 && endOffset == textLength) {
		// Replacing whole text or old text was empty
		SetTo(aBuffer, aLength);
	}

	else if (aOffset == textLength) {
		// Appending to existing
		Append(aBuffer, aLength);
	}

	else {
		// Merging old and new

		// Allocate new buffer
		PRInt32 newLength = textLength - aCount + aLength;
		PRUnichar* to;
		NS_ENSURE_TRUE(to, NS_ERROR_OUT_OF_MEMORY);

		// Copy over appropriate data
		if (aOffset) {
			CopyTo(to, 0, aOffset);
		}
		if (aLength) {
			memcpy(to + aOffset, aBuffer, aLength * sizeof(PRUnichar));
		}
		if (endOffset != textLength) {
			CopyTo(to + aOffset + aLength, endOffset, textLength - endOffset);
		}
	}

	return r;

}
