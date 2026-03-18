import json
import sys
import re
from html.parser import HTMLParser

class TextExtractor(HTMLParser):
    def __init__(self):
        super().__init__()
        self.in_wiki = False
        self.in_article = False
        self.content = []
        self.skip_tags = {'script', 'style', 'nav', 'header', 'footer'}
        self.current_skip = False
        self.tag_stack = []
        
    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        # Check for wiki content area
        if tag == 'div':
            class_attr = attrs_dict.get('class', '')
            if 'wiki-body' in class_attr or 'markdown-body' in class_attr:
                self.in_wiki = True
        if tag == 'article':
            self.in_article = True
        
        self.tag_stack.append(tag)
        if tag in self.skip_tags:
            self.current_skip = True
            
    def handle_endtag(self, tag):
        if self.tag_stack and self.tag_stack[-1] == tag:
            self.tag_stack.pop()
        if tag in self.skip_tags:
            self.current_skip = False
        if tag == 'article':
            self.in_article = False
            
    def handle_data(self, data):
        if self.current_skip:
            return
        # Only get content from wiki area
        if self.in_wiki or self.in_article:
            text = data.strip()
            if text and len(text) > 1:
                self.content.append(text)

def extract_text(html):
    parser = TextExtractor()
    parser.feed(html)
    return '\n'.join(parser.content)

if __name__ == '__main__':
    filename = sys.argv[1]
    with open(filename, 'r') as f:
        data = json.load(f)
    
    html = data.get('data', {}).get('html', '')
    title = data.get('data', {}).get('title', 'Unknown')
    
    text = extract_text(html)
    print(f"=== {title} ===")
    print(text)
    print("\n" + "="*50 + "\n")
