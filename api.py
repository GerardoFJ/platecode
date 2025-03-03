from flask import Flask, request, jsonify
import os
import easyocr
import uuid
import cv2
import numpy as np

app = Flask(__name__)
UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
reader = easyocr.Reader(['en'])

@app.route('/readnumberplate', methods=['POST'])
def upload_image():
    print("Post received")
    if 'imageFile' not in request.files:
        return jsonify({'error': 'No imageFile part'}), 400
    
    file = request.files['imageFile']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400
    
    new_filename = f"{uuid.uuid4()}.jpeg"
    file_path = os.path.join(UPLOAD_FOLDER, new_filename)
    file.save(file_path)
    
    # Load image with OpenCV
    image = cv2.imread(file_path)
    
    # Use EasyOCR to extract text and bounding boxes
    results = reader.readtext(file_path)
    extracted_text = []
    
    for (bbox, text, _) in results:
        extracted_text.append(text)
        (top_left, top_right, bottom_right, bottom_left) = bbox
        top_left = tuple(map(int, top_left))
        bottom_right = tuple(map(int, bottom_right))
        
        # Draw bounding box around detected text
        cv2.rectangle(image, top_left, bottom_right, (0, 255, 0), 2)
        cv2.putText(image, text, (top_left[0], top_left[1] - 10), 
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
    
    # Save the image with bounding boxes
    processed_filename = f"processed_{new_filename}"
    processed_file_path = os.path.join(UPLOAD_FOLDER, processed_filename)
    cv2.imwrite(processed_file_path, image)
    
    return jsonify({'text': " ".join(extracted_text)}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', ssl_context=('cert.pem', 'key.pem'), debug=True)
