#include<bits/stdc++.h>
using namespace std;

class DocElement {
 public:
    virtual string render()=0;
};

class TextElement: public DocElement {
 private:
    string text;
 public:
    TextElement(string text) {
        this->text = text;
    }
    string render() override {
        return "This is a text element: " + this->text;
    }
};

class ImageElement: public DocElement {
 private:
    string imagePath;
 public:
    ImageElement(string imagePath) {
        this->imagePath = imagePath;
    }
    string render() override {
        return "You can see the image here, [" + this->imagePath + "]";
    }
};

class Document {
 private:
    vector<DocElement*> elements;

 public:
    Document() {
        this->elements = vector<DocElement*>();
    }

    void addElement(DocElement* element){
        this->elements.push_back(element);
    }

    void renderDocument(){
        for(DocElement* element: this->elements){
            cout<<element->render()<<endl;
        }
    }
};

class PersistenceManager {
 public:
    virtual void saveDocument(Document* document)=0;
};

class FilePersistenceManager: public PersistenceManager {
 public:
    void saveDocument(Document* document) override {
        cout<<"Saving document to file..."<<endl;
    }
};

class DatabasePersistenceManager: public PersistenceManager {
 public:
    void saveDocument(Document* document) override {
        cout<<"Saving document to database...";
    }
};

class DocumentEditor {
 private:
    Document* document;
    PersistenceManager* persistenceManager;

 public:
    DocumentEditor(Document* document, PersistenceManager* persistenceManager) {
        this->document = document;
        this->persistenceManager = persistenceManager;
    }

    void addTextElement(string text) {
        DocElement* textElement = new TextElement(text);
        this->document->addElement(textElement);
    }

    void addImageElement(string imagePath) {
        DocElement* imageElement = new ImageElement(imagePath);
        this->document->addElement(imageElement);
    }

    void saveDocument() {
        this->persistenceManager->saveDocument(this->document);
    }
};

int main(){

    Document* document = new Document();
    PersistenceManager* persistenceManager = new FilePersistenceManager();
    DocumentEditor* editor = new DocumentEditor(document, persistenceManager);

    editor->addTextElement("Hello, World!");
    editor->addImageElement("/path/to/image.jpg");
    document->renderDocument();

    editor->saveDocument();

    return 0;
}